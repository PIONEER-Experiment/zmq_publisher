#include "wfd5_payload_generator.h"
#include "synthetic_signal.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <endian.h>
#include <limits>
#include <stdexcept>

namespace zmq_publisher::benchmarks {
namespace {

constexpr std::size_t kMaximumWaveformUnits = 0x3fff;
constexpr std::size_t kFixedWords = 3 + 2 + 3 + 1;
constexpr std::size_t kWaveformHeaderWords = 2;

void append_big_endian(std::vector<std::byte>& output, std::uint64_t value) {
    value = htobe64(value);
    const auto old_size = output.size();
    output.resize(old_size + sizeof(value));
    std::memcpy(output.data() + old_size, &value, sizeof(value));
}

void append_samples(
    std::vector<std::byte>& output,
    std::size_t words,
    std::uint64_t sequence,
    std::size_t waveform_index) {
    const auto signal_seed = SyntheticSignal::seed(0x57464435ULL, sequence, waveform_index);
    const auto sample_count = words * 4U;
    const auto nominal_onset = std::min(128.0, std::max(4.0, sample_count * 0.25));
    const auto onset = nominal_onset + SyntheticSignal::trigger_jitter(signal_seed, 2.0);
    const auto rise_time = 3.0 + 2.0 * SyntheticSignal::uniform(signal_seed + 1U);
    const auto decay_time = 32.0 + 16.0 * SyntheticSignal::uniform(signal_seed + 2U);
    const auto amplitude = 300.0 + 1700.0 * SyntheticSignal::uniform(signal_seed + 3U);
    for (std::size_t word = 0; word < words; ++word) {
        std::array<std::int16_t, 4> samples{};
        for (std::size_t sample = 0; sample < samples.size(); ++sample) {
            const auto sample_index = word * samples.size() + sample;
            const auto value = 100.0 +
                SyntheticSignal::noise(
                    SyntheticSignal::seed(
                        0x57464435ULL, sequence, waveform_index, sample_index), 10.0) +
                SyntheticSignal::double_exponential_pulse(
                    sample_index, onset, rise_time, decay_time, amplitude);
            samples[sample] = static_cast<std::int16_t>(
                std::clamp(value, -32768.0, 32767.0));
        }
        const auto old_size = output.size();
        output.resize(old_size + sizeof(samples));
        std::memcpy(output.data() + old_size, samples.data(), sizeof(samples));
    }
}

}  // namespace

std::string_view Wfd5PayloadGenerator::detector_name() const noexcept {
    return "wfd5";
}

std::vector<DetectorBank> Wfd5PayloadGenerator::generate(
    std::size_t requested_payload_bytes,
    std::uint64_t sequence) const {
    const auto requested_words = std::max<std::size_t>(
        (requested_payload_bytes + sizeof(std::uint64_t) - 1) / sizeof(std::uint64_t),
        kFixedWords + kWaveformHeaderWords + 2);
    const auto desired_sample_words = requested_words - kFixedWords - kWaveformHeaderWords;
    const auto waveform_count = std::max<std::size_t>(
        1, (desired_sample_words + 2 * kMaximumWaveformUnits - 1) /
               (2 * kMaximumWaveformUnits));
    if (waveform_count > 0xfff) {
        throw std::invalid_argument("Requested WFD5 payload needs too many waveforms");
    }

    std::vector<std::size_t> waveform_units(waveform_count, 1);
    auto remaining_words = desired_sample_words > 2 * waveform_count
        ? desired_sample_words - 2 * waveform_count
        : 0;
    for (auto& units : waveform_units) {
        const auto additional_units = std::min(
            remaining_words / 2, kMaximumWaveformUnits - 1);
        units += additional_units;
        remaining_words -= additional_units * 2;
    }

    const auto total_words = kFixedWords + kWaveformHeaderWords * waveform_count +
        2 * [&] {
            std::size_t total = 0;
            for (const auto units : waveform_units) total += units;
            return total;
        }();
    if (total_words > 0xfffff) {
        throw std::invalid_argument("Requested WFD5 payload exceeds AMC length field");
    }

    DetectorBank data{{'C', 'R', '0', '0', '\0'}, {}};
    data.bytes.reserve(total_words * sizeof(std::uint64_t));

    // AMC header: slot 1, one enabled channel, synchronous mode, board type WFD5.
    append_big_endian(data.bytes,
        (std::uint64_t{1} << 56) |
        ((sequence & 0xffffffU) << 32) |
        (sequence & 0xfffU) << 20 |
        total_words);
    append_big_endian(data.bytes,
        (std::uint64_t{1} << 59) |
        (std::uint64_t{1} << 13) |
        ((sequence & 0xffffffffU) << 16) |
        1);
    append_big_endian(data.bytes, std::uint64_t{1} << 16);

    constexpr std::uint64_t channel_tag = 1;
    append_big_endian(data.bytes,
        (channel_tag << 46) |
        (std::uint64_t{16} << 24) |
        (waveform_count << 12));
    append_big_endian(data.bytes,
        (waveform_units.front() << 27) |
        (sequence & 0xffffffU));

    for (std::size_t index = 0; index < waveform_units.size(); ++index) {
        const auto units = waveform_units[index];
        append_big_endian(data.bytes,
            (index << 52) |
            (std::uint64_t{16} << 14) |
            units);
        append_big_endian(data.bytes, channel_tag << 34);
        append_samples(data.bytes, units * 2, sequence, index);
    }

    // Three channel-trailer words followed by the AMC trailer.
    append_big_endian(data.bytes, 0);
    append_big_endian(data.bytes, 0);
    append_big_endian(data.bytes, 0);
    append_big_endian(data.bytes,
        ((sequence & 0xffU) << 24) | total_words);
    return {std::move(data)};
}

}  // namespace zmq_publisher::benchmarks
