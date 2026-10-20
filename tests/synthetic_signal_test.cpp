#include "synthetic_signal.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>

TEST_CASE("synthetic detector pulse has a fast rise and slower decay") {
    constexpr double onset = 16.0;
    constexpr double amplitude = 200.0;
    std::array<double, 64> samples{};
    for (std::size_t index = 0; index < samples.size(); ++index) {
        samples[index] = zmq_publisher::benchmarks::SyntheticSignal::double_exponential_pulse(
            index, onset, 1.0, 8.0, amplitude);
    }

    CHECK(samples[15] == 0.0);
    CHECK(samples[16] == 0.0);
    const auto peak = std::max_element(samples.begin(), samples.end());
    CHECK(std::distance(samples.begin(), peak) >= 17);
    CHECK(std::distance(samples.begin(), peak) <= 20);
    CHECK(*peak == Catch::Approx(amplitude).margin(3.0));
    CHECK(samples[40] > 0.0);
    CHECK(samples[40] < samples[24]);
}

TEST_CASE("synthetic electronics noise is deterministic and centered") {
    using zmq_publisher::benchmarks::SyntheticSignal;
    CHECK(SyntheticSignal::noise(42, 3.0) == SyntheticSignal::noise(42, 3.0));

    double sum = 0.0;
    constexpr std::size_t count = 10'000;
    for (std::size_t index = 0; index < count; ++index) {
        sum += SyntheticSignal::noise(index, 3.0);
    }
    CHECK(std::abs(sum / count) < 0.1);
}
