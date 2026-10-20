#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace zmq_publisher::benchmarks {

class SyntheticSignal {
public:
    [[nodiscard]] static double uniform(std::uint64_t seed) noexcept {
        seed += 0x9e3779b97f4a7c15ULL;
        seed = (seed ^ (seed >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        seed = (seed ^ (seed >> 27U)) * 0x94d049bb133111ebULL;
        seed ^= seed >> 31U;
        return static_cast<double>(seed >> 11U) * (1.0 / 9007199254740992.0);
    }

    [[nodiscard]] static double noise(std::uint64_t seed, double standard_deviation) noexcept {
        // Box-Muller gives the electronics noise a Gaussian distribution while
        // preserving deterministic output for a given event and channel.
        const auto first = std::max(uniform(seed), std::numeric_limits<double>::min());
        const auto second = uniform(seed ^ 0xd1b54a32d192ed03ULL);
        constexpr double two_pi = 6.28318530717958647692;
        return standard_deviation * std::sqrt(-2.0 * std::log(first)) *
            std::cos(two_pi * second);
    }

    [[nodiscard]] static double double_exponential_pulse(
        std::size_t sample,
        double onset,
        double rise_time,
        double decay_time,
        double amplitude) noexcept {
        const auto position = static_cast<double>(sample);
        if (position <= onset || rise_time <= 0.0 || decay_time <= rise_time) return 0.0;

        const auto elapsed = position - onset;
        const auto peak_time = rise_time * decay_time / (decay_time - rise_time) *
            std::log(decay_time / rise_time);
        const auto normalization =
            std::exp(-peak_time / decay_time) - std::exp(-peak_time / rise_time);
        const auto response =
            std::exp(-elapsed / decay_time) - std::exp(-elapsed / rise_time);
        return amplitude * response / normalization;
    }

    [[nodiscard]] static double trigger_jitter(
        std::uint64_t seed,
        double maximum_offset) noexcept {
        return maximum_offset * (2.0 * uniform(seed) - 1.0);
    }

    [[nodiscard]] static std::uint64_t seed(
        std::uint64_t detector,
        std::uint64_t sequence,
        std::uint64_t channel,
        std::uint64_t sample = 0) noexcept {
        return detector ^ (sequence * 0x9e3779b97f4a7c15ULL) ^
            (channel * 0xbf58476d1ce4e5b9ULL) ^ sample;
    }
};

}  // namespace zmq_publisher::benchmarks
