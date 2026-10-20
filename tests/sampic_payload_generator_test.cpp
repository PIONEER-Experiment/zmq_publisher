#include "sampic_payload_generator.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstring>
#include <set>
#include <string_view>

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

TEST_CASE("SAMPIC online DQM simulator varies multiplicity while covering ATAR boards") {
    const SampicPayloadGenerator generator(SampicSimulationMode::time_grouping, true);
    const auto first = generator.generate(16 * 1024, 1);
    const auto second = generator.generate(16 * 1024, 9);
    REQUIRE(find_bank(first, "SD00") != nullptr);
    REQUIRE(find_bank(second, "SD00") != nullptr);
    CHECK(find_bank(first, "SD00")->bytes.size() != find_bank(second, "SD00")->bytes.size());
    std::set<std::int32_t> boards;
    const auto& data = find_bank(first, "SD00")->bytes;
    for (std::size_t offset = 0; offset + 344 <= data.size(); offset += 344) {
        std::int32_t board = -1;
        std::memcpy(&board, data.data() + offset, sizeof(board));
        boards.insert(board);
    }
    CHECK(boards == std::set<std::int32_t>{0, 1, 2, 3});
}
