#include "rate_subscriber.h"

#include <chrono>
#include <cstring>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>
#include <zmq.hpp>

namespace zmq_publisher::benchmarks {
namespace {

constexpr std::uint32_t kTimingMagic = 0x5254494d;
constexpr std::uint32_t kBank32 = 1U << 4U;
constexpr std::uint32_t kBank32A = 1U << 5U;

template <typename Value>
Value read_value(const std::byte* data, std::size_t size, std::size_t offset) {
    if (offset + sizeof(Value) > size) {
        throw std::runtime_error("Truncated MIDAS event");
    }
    Value value{};
    std::memcpy(&value, data + offset, sizeof(Value));
    return value;
}

std::int64_t steady_now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <typename Json>
void write_optional(std::ofstream& output, const Json& object, std::string_view key) {
    const auto found = object.find(key);
    if (found != object.end() && !found->is_null()) {
        output << found->template get<std::int64_t>();
    }
}

}  // namespace

RateSubscriber::RateSubscriber(std::string endpoint, std::string output_path)
    : endpoint_(std::move(endpoint)), output_(std::move(output_path)) {
    if (!output_) {
        throw std::runtime_error("Could not open native subscriber output");
    }
    write_header();
}

void RateSubscriber::write_header() {
    output_ << "received_monotonic_ns,producer_monotonic_ns,receiver_monotonic_ns,"
               "processor_dequeued_ns,product_created_ns,published_ns,source_sequence,"
               "publish_sequence,payload_bytes,product_bytes,receiver_events_received,"
               "receiver_events_consumed,receiver_events_dropped,receiver_queue_depth,"
               "receiver_batch_queue_depth,receiver_queue_high_watermark,"
               "receiver_total_queue_latency_ns\n";
}

RateSubscriber::SourceTiming RateSubscriber::parse_source_timing(
    const std::byte* data,
    std::size_t size) {
    if (size < 24) {
        throw std::runtime_error("MIDAS event is shorter than its headers");
    }
    const auto event_data_size = read_value<std::uint32_t>(data, size, 12);
    const auto bank_data_size = read_value<std::uint32_t>(data, size, 16);
    const auto flags = read_value<std::uint32_t>(data, size, 20);
    const auto end = std::min(size, std::min<std::size_t>(
        24 + bank_data_size,
        16 + event_data_size));
    std::size_t position = 24;
    while (position < end) {
        if (position + 8 > end) {
            break;
        }
        const auto name = read_value<std::uint32_t>(data, end, position);
        std::size_t header_size = 8;
        std::size_t value_size = 0;
        if ((flags & kBank32A) != 0U) {
            header_size = 16;
            value_size = read_value<std::uint32_t>(data, end, position + 8);
        } else if ((flags & kBank32) != 0U) {
            header_size = 12;
            value_size = read_value<std::uint32_t>(data, end, position + 8);
        } else {
            value_size = read_value<std::uint16_t>(data, end, position + 6);
        }
        const auto value_offset = position + header_size;
        if (name == 0x4d495452U && value_size >= 40) {  // "RTIM" on little-endian hosts
            if (read_value<std::uint32_t>(data, end, value_offset) != kTimingMagic) {
                break;
            }
            return {
                read_value<std::uint64_t>(data, end, value_offset + 8),
                read_value<std::uint64_t>(data, end, value_offset + 16),
                read_value<std::uint32_t>(data, end, value_offset + 32),
            };
        }
        position = value_offset + ((value_size + 7U) & ~std::size_t{7U});
    }
    throw std::runtime_error("MIDAS event does not contain an RTIM bank");
}

void RateSubscriber::run(const std::atomic<bool>& running) {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.set(zmq::sockopt::subscribe, "DATA");
    subscriber.set(zmq::sockopt::rcvhwm, 1'000'000);
    subscriber.set(zmq::sockopt::rcvtimeo, 100);
    subscriber.connect(endpoint_);

    std::uint64_t rows_since_flush = 0;
    while (running.load(std::memory_order_relaxed)) {
        std::vector<zmq::message_t> frames;
        zmq::message_t frame;
        const auto received = subscriber.recv(frame, zmq::recv_flags::none);
        if (!received) {
            continue;
        }
        frames.push_back(std::move(frame));
        while (subscriber.get(zmq::sockopt::rcvmore)) {
            zmq::message_t next;
            if (!subscriber.recv(next, zmq::recv_flags::none)) {
                break;
            }
            frames.push_back(std::move(next));
        }
        const auto received_ns = steady_now_ns();
        if (frames.size() < 3) {
            continue;
        }
        try {
            const auto metadata = nlohmann::json::parse(
                static_cast<const char*>(frames[1].data()),
                static_cast<const char*>(frames[1].data()) + frames[1].size());
            const auto& products = metadata.at("products");
            const auto count = std::min(products.size(), frames.size() - 2);
            for (std::size_t index = 0; index < count; ++index) {
                const auto& product = products[index];
                const auto& markers = product.at("timing-markers");
                const auto& attributes = product.at("numeric-attributes");
                const auto timing = parse_source_timing(
                    static_cast<const std::byte*>(frames[index + 2].data()),
                    frames[index + 2].size());
                const auto created = product.at("created-at-steady-ns").get<std::int64_t>();
                output_ << received_ns << ',' << timing.monotonic_ns << ',';
                write_optional(output_, markers, "midas-receiver");
                output_ << ',';
                write_optional(output_, markers, "midas-processor-dequeued");
                output_ << ',' << created << ','
                        << metadata.at("published-at-steady-ns").get<std::int64_t>() << ','
                        << timing.sequence << ','
                        << metadata.at("sequence").get<std::uint64_t>() << ','
                        << timing.payload_bytes << ','
                        << product.at("size").get<std::uint64_t>() << ',';
                for (const auto key : {
                         "midas-receiver.events-received",
                         "midas-receiver.events-consumed",
                         "midas-receiver.events-dropped",
                         "midas-receiver.queue-depth",
                         "midas-receiver.batch-queue-depth",
                         "midas-receiver.queue-high-watermark",
                         "midas-receiver.total-queue-latency-ns"}) {
                    write_optional(output_, attributes, key);
                    if (key != std::string_view("midas-receiver.total-queue-latency-ns")) {
                        output_ << ',';
                    }
                }
                output_ << '\n';
                if (++rows_since_flush >= 4096) {
                    output_.flush();
                    rows_since_flush = 0;
                }
            }
        } catch (const std::exception&) {
            // Malformed/non-benchmark products are irrelevant to this rate subscriber.
        }
    }
    output_.flush();
}

}  // namespace zmq_publisher::benchmarks
