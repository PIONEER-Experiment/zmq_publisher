#include "hdsoc_payload_generator.h"
#include "synthetic_signal.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <endian.h>
#include <limits>
#include <stdexcept>

namespace zmq_publisher::benchmarks {
namespace {

#pragma pack(push, 1)
struct EventHeader {
    std::uint16_t event_header;
    std::uint8_t event_info;
    std::uint32_t event_index;
    std::uint32_t event_reference_time;
    std::uint32_t time_threshold;
    std::uint32_t event_completion_time_us;
    std::uint32_t clock_frequency;
    std::uint16_t packet_size;
    std::uint64_t channel_mask;
    std::uint8_t num_windows;
    std::uint16_t num_packets;
};

struct Packet {
    std::uint16_t packet_header;
    std::uint8_t packet_info;
    std::uint8_t channel;
    std::uint32_t trigger_time;
    std::uint16_t logical_position;
    std::uint16_t window_position;
    std::array<std::uint16_t, 32> trace;
    std::uint16_t parser_index;
    std::uint16_t packet_footer;
};

struct EventFooter {
    std::uint16_t event_footer;
};

struct TimeData {
    std::uint64_t collection_cycle_index;
    std::uint64_t collection_cycle_timestamp_ns;
    double udp_time;
    double parse_time;
    double event_time;
    double total_time;
    std::uint64_t data_processed;
    double data_rate;
};
#pragma pack(pop)

template <typename Value>
void append(std::vector<std::byte>& output, const Value& value) {
    const auto old_size = output.size();
    output.resize(old_size + sizeof(Value));
    std::memcpy(output.data() + old_size, &value, sizeof(Value));
}

}  // namespace

std::string_view HdsocPayloadGenerator::detector_name() const noexcept {
    return "hdsoc";
}

std::vector<DetectorBank> HdsocPayloadGenerator::generate(
    std::size_t requested_payload_bytes,
    std::uint64_t sequence) const {
    constexpr std::size_t fixed_bytes = sizeof(EventHeader) + sizeof(EventFooter);
    const auto usable = requested_payload_bytes > fixed_bytes
        ? requested_payload_bytes - fixed_bytes
        : sizeof(Packet);
    const auto packet_count = std::clamp<std::size_t>(
        usable / sizeof(Packet), 1, std::numeric_limits<std::uint16_t>::max());

    EventHeader header{
        0xbbbb,
        0,
        static_cast<std::uint32_t>(sequence),
        static_cast<std::uint32_t>(sequence),
        0,
        0,
        1'000'000'000U,
        sizeof(Packet),
        std::numeric_limits<std::uint64_t>::max(),
        static_cast<std::uint8_t>(std::min<std::size_t>(
            0xff, (packet_count + 63) / 64)),
        static_cast<std::uint16_t>(packet_count),
    };
    DetectorBank data{{'H', 'D', '0', '0', '\0'}, {}};
    data.bytes.reserve(fixed_bytes + packet_count * sizeof(Packet));
    append(data.bytes, header);
    for (std::size_t index = 0; index < packet_count; ++index) {
        const auto channel = index % 64;
        const auto window = index / 64;
        Packet packet{};
        packet.packet_header = 0xaaaa;
        packet.channel = static_cast<std::uint8_t>(channel);
        packet.trigger_time = static_cast<std::uint32_t>(sequence);
        packet.logical_position = static_cast<std::uint16_t>(window);
        packet.window_position = static_cast<std::uint16_t>(window);
        packet.parser_index = static_cast<std::uint16_t>(index);
        packet.packet_footer = 0xffff;
        const auto signal_seed = SyntheticSignal::seed(0x4844534f43ULL, sequence, channel);
        const auto onset = 8.0 + SyntheticSignal::trigger_jitter(signal_seed, 0.75);
        const auto rise_time = 0.65 + 0.35 * SyntheticSignal::uniform(signal_seed + 1U);
        const auto decay_time = 4.0 + 2.0 * SyntheticSignal::uniform(signal_seed + 2U);
        const auto amplitude = 100.0 + 700.0 * SyntheticSignal::uniform(signal_seed + 3U);
        for (std::size_t sample = 0; sample < packet.trace.size(); ++sample) {
            const auto waveform_sample = window * packet.trace.size() + sample;
            const auto value = 2048.0 +
                SyntheticSignal::noise(
                    SyntheticSignal::seed(
                        0x4844534f43ULL, sequence, channel, waveform_sample), 8.0) +
                SyntheticSignal::double_exponential_pulse(
                    waveform_sample, onset, rise_time, decay_time, amplitude);
            // The HDSoC payload stores ADC words in network byte order; the
            // shared unpacker converts each sample back to host order.
            packet.trace[sample] = htobe16(static_cast<std::uint16_t>(
                std::clamp(value, 0.0, 4095.0)));
        }
        append(data.bytes, packet);
    }
    append(data.bytes, EventFooter{0xeeee});

    const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const TimeData timing{
        sequence,
        static_cast<std::uint64_t>(timestamp),
        0.0,
        0.0,
        0.0,
        0.0,
        static_cast<std::uint64_t>(data.bytes.size()),
        0.0,
    };
    DetectorBank time{{'H', 'T', '0', '0', '\0'}, {}};
    append(time.bytes, timing);
    return {std::move(data), std::move(time)};
}

}  // namespace zmq_publisher::benchmarks
