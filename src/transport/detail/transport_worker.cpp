#include "transport_worker.h"

#include "zmq_publisher/metrics/component_metrics.h"

#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <zmq.hpp>

namespace zmq_publisher {

namespace {

void release_payload(void*, void* hint) {
    delete static_cast<ImmutablePayload*>(hint);
}

zmq::message_t payload_frame(const ImmutablePayload& payload) {
    if (payload.empty()) {
        return zmq::message_t{};
    }
    auto* owner = new ImmutablePayload(payload);
    return zmq::message_t(
        const_cast<std::byte*>(payload.data()), payload.size(), release_payload, owner);
}

nlohmann::json envelope(
    const PublishMessage& message,
    std::chrono::steady_clock::time_point published_at) {
    const auto published_at_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        published_at.time_since_epoch()).count();
    nlohmann::json result{
        {"protocol", "zmq-publisher-v1"},
        {"sequence", message.sequence},
        {"published-at-steady-ns", published_at_ns},
        {"products", nlohmann::json::array()},
    };
    for (const auto& product : message.products) {
        const auto created_at_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            product.created_at().time_since_epoch()).count();
        nlohmann::json product_metadata{
            {"sequence", product.sequence()},
            {"content-type", product.content_type()},
            {"schema", product.schema()},
            {"size", product.size()},
            {"created-at-steady-ns", created_at_ns},
            {"timing-markers", nlohmann::json::object()},
            {"numeric-attributes", product.numeric_attributes()},
        };
        for (const auto& marker : product.timing_markers()) {
            product_metadata["timing-markers"][marker.stage] =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    marker.timestamp.time_since_epoch()).count();
        }
        result["products"].push_back(std::move(product_metadata));
    }
    return result;
}

}  // namespace

TransportWorker::TransportWorker(
    std::size_t id,
    std::size_t queue_capacity,
    QueuePolicy queue_policy,
    std::size_t zmq_io_threads,
    std::shared_ptr<ComponentMetrics> metrics)
    : id_(id),
      queue_capacity_(queue_capacity),
      queue_policy_(queue_policy),
      zmq_io_threads_(zmq_io_threads),
      metrics_(std::move(metrics)) {}

TransportWorker::~TransportWorker() {
    stop();
}

void TransportWorker::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    thread_ = std::thread(&TransportWorker::io_loop, this);
}

void TransportWorker::stop() {
    {
        std::lock_guard lock(mutex_);
        running_ = false;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

PublishResult TransportWorker::publish(PublishMessage message) {
    const auto wait_started = std::chrono::steady_clock::now();
    std::unique_lock lock(mutex_);
    if (!running_) {
        return PublishResult::stopped;
    }

    if (queue_policy_ == QueuePolicy::block) {
        not_full_.wait(lock, [this] { return !running_ || queue_.size() < queue_capacity_; });
        metrics_->record_blocked(std::chrono::steady_clock::now() - wait_started);
        if (!running_) {
            return PublishResult::stopped;
        }
    } else if (queue_.size() >= queue_capacity_) {
        if (queue_policy_ == QueuePolicy::drop_newest) {
            metrics_->record_drop();
            return PublishResult::dropped;
        }
        if (queue_policy_ == QueuePolicy::fail) {
            throw std::runtime_error("Transport worker queue is full");
        }
        queue_.pop_front();
        metrics_->record_drop();
    }

    queue_.push_back(std::move(message));
    metrics_->set_queue_depth(queue_.size());
    lock.unlock();
    not_empty_.notify_one();
    return PublishResult::queued;
}

void TransportWorker::io_loop() {
    zmq::context_t context(static_cast<int>(zmq_io_threads_));
    std::unordered_map<std::string, std::unique_ptr<zmq::socket_t>> sockets;

    while (true) {
        PublishMessage message;
        {
            std::unique_lock lock(mutex_);
            not_empty_.wait(lock, [this] { return !running_ || !queue_.empty(); });
            if (queue_.empty()) {
                if (!running_) {
                    break;
                }
                continue;
            }
            message = std::move(queue_.front());
            queue_.pop_front();
            metrics_->set_queue_depth(queue_.size());
        }
        not_full_.notify_one();

        try {
            auto found = sockets.find(message.endpoint);
            if (found == sockets.end()) {
                auto socket = std::make_unique<zmq::socket_t>(context, zmq::socket_type::pub);
                socket->bind(message.endpoint);
                found = sockets.emplace(message.endpoint, std::move(socket)).first;
                spdlog::info("Transport worker {} bound ZMQ publisher to {}", id_, message.endpoint);
            }

            const auto published_at = std::chrono::steady_clock::now();
            const auto metadata = envelope(message, published_at).dump();
            found->second->send(zmq::buffer(message.topic), zmq::send_flags::sndmore);
            found->second->send(zmq::buffer(metadata),
                                message.products.empty()
                                    ? zmq::send_flags::none
                                    : zmq::send_flags::sndmore);
            std::size_t bytes = metadata.size();
            for (std::size_t index = 0; index < message.products.size(); ++index) {
                auto frame = payload_frame(message.products[index].payload());
                bytes += frame.size();
                found->second->send(
                    std::move(frame),
                    index + 1 == message.products.size()
                        ? zmq::send_flags::none
                        : zmq::send_flags::sndmore);
            }
            metrics_->record_invocation();
            metrics_->record_items(message.products.size());
            metrics_->record_bytes(bytes);
            metrics_->record_latency(std::chrono::steady_clock::now() - message.created_at);
        } catch (const zmq::error_t& error) {
            metrics_->record_failure();
            spdlog::error("Failed to publish to '{}': {}", message.endpoint, error.what());
            sockets.erase(message.endpoint);
        }
    }

    sockets.clear();
    context.shutdown();
    context.close();
}

}  // namespace zmq_publisher
