#include "sampic_payload_generator.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <string_view>
#include <vector>

namespace {

using zmq_publisher::benchmarks::DetectorBank;
using zmq_publisher::benchmarks::SampicPayloadGenerator;
using zmq_publisher::benchmarks::SampicSimulationMode;

const DetectorBank* find_bank(const std::vector<DetectorBank>& banks, std::string_view name) {
    const auto found = std::find_if(banks.begin(), banks.end(), [name](const auto& bank) {
        return std::string_view(bank.name.data()) == name;
    });
    return found == banks.end() ? nullptr : &*found;
}

}  // namespace

TEST_CASE("SAMPIC time-grouping simulator emits compact hit metadata") {
    const auto banks = SampicPayloadGenerator(SampicSimulationMode::time_grouping).generate(344, 7);
    REQUIRE(find_bank(banks, "SD00") != nullptr);
    REQUIRE(find_bank(banks, "SH00") != nullptr);
    REQUIRE(find_bank(banks, "ST00") != nullptr);
    REQUIRE(find_bank(banks, "SC00") != nullptr);
    CHECK(find_bank(banks, "SH00")->bytes.size() == 8 + 54);
}

TEST_CASE("SAMPIC external-trigger simulator emits trigger association") {
    const auto banks = SampicPayloadGenerator(SampicSimulationMode::external_gated_trigger).generate(344, 7);
    REQUIRE(find_bank(banks, "SG00") != nullptr);
    CHECK(find_bank(banks, "SG00")->bytes.size() == 36);
    CHECK(find_bank(banks, "SC00") == nullptr);
    CHECK(find_bank(banks, "SV00") == nullptr);
}

TEST_CASE("SAMPIC vendor-passthrough simulator emits vendor triggers") {
    const auto banks = SampicPayloadGenerator(SampicSimulationMode::vendor_passthrough).generate(344, 7);
    REQUIRE(find_bank(banks, "SV00") != nullptr);
    CHECK(find_bank(banks, "SV00")->bytes.size() == 4 + 20);
    CHECK(find_bank(banks, "SC00") == nullptr);
    CHECK(find_bank(banks, "SG00") == nullptr);
}

TEST_CASE("SAMPIC online DQM simulator streams one decay time group per MIDAS event") {
    const SampicPayloadGenerator generator(SampicSimulationMode::time_grouping, true);
    std::set<std::int32_t> boards;
    std::set<std::size_t> hit_counts;
    std::array<std::set<std::int32_t>, 8> strips_by_layer;
    for (std::uint64_t sequence = 1; sequence <= 24; ++sequence) {
        const auto banks = generator.generate(16 * 1024, sequence);
        REQUIRE(find_bank(banks, "SD00") != nullptr);
        const auto& data = find_bank(banks, "SD00")->bytes;
        hit_counts.insert(data.size() / 344);
        std::vector<double> event_times;
        for (std::size_t offset = 0; offset + 344 <= data.size(); offset += 344) {
            std::int32_t board = -1;
            std::int32_t channel = -1;
            double time_nanoseconds = 0.0;
            std::memcpy(&board, data.data() + offset, sizeof(board));
            std::memcpy(&channel, data.data() + offset + sizeof(board), sizeof(channel));
            std::memcpy(&time_nanoseconds, data.data() + offset + 324, sizeof(time_nanoseconds));
            boards.insert(board);
            const auto global_channel = board * 64 + channel;
            strips_by_layer.at(static_cast<std::size_t>(global_channel / 32))
                .insert(global_channel % 32);
            event_times.push_back(time_nanoseconds);
        }
        REQUIRE_FALSE(event_times.empty());
        const auto [minimum, maximum] = std::minmax_element(
            event_times.begin(), event_times.end());
        CHECK(*maximum - *minimum < 12.0);
    }
    CHECK(boards == std::set<std::int32_t>{0, 1, 2, 3});
    CHECK(hit_counts.size() > 1);
    for (const auto& strips : strips_by_layer) {
        REQUIRE_FALSE(strips.empty());
    }
}
