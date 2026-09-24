#include "zmq_publisher/channel/channel.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("a channel batches accepted processor output") {
    zmq_publisher::Channel channel({"test", "TOPIC", "inproc://test", 1, 1, 2});

    auto first = channel.accept({"one"});
    REQUIRE(first);
    CHECK(first->topic == "TOPIC");
    CHECK(nlohmann::json::parse(first->payload) == nlohmann::json::array({"one"}));

    auto ignored = channel.accept({"two"});
    CHECK_FALSE(ignored);

    auto third = channel.accept({"three"});
    REQUIRE(third);
    CHECK(nlohmann::json::parse(third->payload) ==
          nlohmann::json::array({"two", "three"}));
}
