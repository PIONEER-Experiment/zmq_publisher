#include "musip_payload_generator.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>

namespace {

std::uint64_t word_at(const std::vector<std::byte>& bytes, std::size_t index) {
    std::uint64_t result = 0;
    std::memcpy(&result, bytes.data() + index * sizeof(result), sizeof(result));
    return result;
}

}  // namespace

TEST_CASE("MuSiP simulator emits changing independent hit observations") {
    const zmq_publisher::benchmarks::MusipPayloadGenerator generator;
    const auto first = generator.generate(1024, 7);
    const auto second = generator.generate(1024, 8);

    REQUIRE(first.size() == 1);
    REQUIRE(first.front().bytes.size() == 1024);
    CHECK(first.front().name[0] == 'H');
    CHECK(first.front().bytes != second.front().bytes);

    const auto pixel = word_at(first.front().bytes, 0);
    const auto mutrig = word_at(first.front().bytes, 1);
    CHECK((pixel >> 63U) == 0);
    CHECK((mutrig >> 63U) == 1);
    CHECK((pixel & 0x1fffffffffULL) != (mutrig & 0x7fffffffffULL));
}
