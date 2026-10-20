#include "musip_payload_generator.h"
#include "synthetic_signal.h"

#include <algorithm>
#include <cstring>

namespace zmq_publisher::benchmarks {
namespace {

void append(std::vector<std::byte>& output, std::uint64_t value) {
    const auto old_size = output.size();
    output.resize(old_size + sizeof(value));
    std::memcpy(output.data() + old_size, &value, sizeof(value));
}

std::uint64_t pixel_hit(std::uint64_t sequence, std::size_t index) {
    const auto hit_index = index / 2;
    const auto channel = (sequence + hit_index) % 16;
    const auto chip = 16 + channel;
    const auto signal_seed = SyntheticSignal::seed(
        0x4d55534950ULL, sequence, channel, hit_index);
    const auto column = static_cast<std::uint64_t>(
        SyntheticSignal::uniform(signal_seed + 1U) * 256.0);
    const auto row = static_cast<std::uint64_t>(
        SyntheticSignal::uniform(signal_seed + 2U) * 256.0);
    const auto value = 12.0 + SyntheticSignal::noise(signal_seed + 3U, 4.0);
    const auto tot = static_cast<std::uint64_t>(std::clamp(value, 1.0, 31.0));
    const auto within_event = static_cast<std::uint64_t>(
        SyntheticSignal::uniform(signal_seed + 4U) * 60000.0);
    const auto timestamp = (sequence * 65536 + within_event) & 0x1fffffffffULL;
    return (chip << 58) | (column << 50) | (row << 42) | (tot << 37) | timestamp;
}

std::uint64_t mutrig_hit(std::uint64_t sequence, std::size_t index) {
    const auto hit_index = index / 2;
    const auto trace_channel = (sequence + hit_index) % 16;
    const std::uint64_t chip = 0;
    const std::uint64_t channel = trace_channel;
    const auto signal_seed = SyntheticSignal::seed(
        0x4d5554524947ULL, sequence, channel, hit_index);
    const auto value = 130.0 + SyntheticSignal::noise(signal_seed + 1U, 45.0);
    const auto energy_time = static_cast<std::uint64_t>(std::clamp(value, 1.0, 510.0));
    const auto within_event = static_cast<std::uint64_t>(
        SyntheticSignal::uniform(signal_seed + 2U) * 60000.0);
    const auto timestamp = (sequence * 65536 + within_event) & 0x7fffffffffULL;
    return (1ULL << 63) | (chip << 61) | (channel << 56) |
           (energy_time << 47) | timestamp;
}

}  // namespace

std::string_view MusipPayloadGenerator::detector_name() const noexcept {
    return "musip";
}

std::vector<DetectorBank> MusipPayloadGenerator::generate(
    std::size_t requested_payload_bytes,
    std::uint64_t sequence) const {
    const auto hit_count = std::max<std::size_t>(
        1, requested_payload_bytes / sizeof(std::uint64_t));
    DetectorBank hits{{'H', '0', '0', '0', '\0'}, {}};
    hits.bytes.reserve(hit_count * sizeof(std::uint64_t));
    for (std::size_t index = 0; index < hit_count; ++index) {
        append(hits.bytes, index % 2 == 0
            ? pixel_hit(sequence, index)
            : mutrig_hit(sequence, index));
    }
    return {std::move(hits)};
}

}  // namespace zmq_publisher::benchmarks
