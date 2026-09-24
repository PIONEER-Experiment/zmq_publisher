#include "zmq_publisher/channel/data_buffer.h"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>

TEST_CASE("a data buffer keeps its newest values") {
    zmq_publisher::DataBuffer<int> buffer(3);
    CHECK(buffer.empty());
    CHECK(buffer.snapshot().empty());

    buffer.push(1);
    buffer.push(2);
    CHECK(buffer.snapshot() == std::vector<int>{1, 2});

    buffer.push(3);
    buffer.push(4);
    CHECK(buffer.size() == 3);
    CHECK(buffer.snapshot() == std::vector<int>{2, 3, 4});
}

TEST_CASE("a data buffer rejects zero capacity") {
    CHECK_THROWS_AS(zmq_publisher::DataBuffer<int>(0), std::invalid_argument);
}
