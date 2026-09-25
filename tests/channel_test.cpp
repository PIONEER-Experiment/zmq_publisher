#include "zmq_publisher/channel/channel.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {

zmq_publisher::OutputMessage text(std::string value) {
    return zmq_publisher::OutputMessage::from_text(value);
}

std::string payload(const zmq_publisher::OutputMessage& message) {
    return {reinterpret_cast<const char*>(message.payload()->data()), message.size()};
}

}  // namespace

TEST_CASE("each delivery publishes products without combining their payloads") {
    zmq_publisher::Channel channel({
        "test", "TOPIC", "inproc://test", zmq_publisher::DeliveryPolicy::each, 1});

    std::vector<zmq_publisher::OutputMessage> output;
    output.push_back(text("one"));
    output.push_back(text("two"));
    const auto messages = channel.accept(std::move(output));

    REQUIRE(messages.size() == 2);
    CHECK(messages.front().topic == "TOPIC");
    REQUIRE(messages.front().products.size() == 1);
    CHECK(payload(messages.front().products.front()) == "one");
    CHECK(payload(messages.back().products.front()) == "two");
}

TEST_CASE("batch delivery moves products into non-overlapping batches") {
    zmq_publisher::Channel channel({
        "test", "TOPIC", "inproc://test", zmq_publisher::DeliveryPolicy::batch, 2});

    CHECK(channel.accept({text("one")}).empty());
    const auto messages = channel.accept({text("two")});
    REQUIRE(messages.size() == 1);
    REQUIRE(messages.front().products.size() == 2);
    CHECK(payload(messages.front().products[0]) == "one");
    CHECK(payload(messages.front().products[1]) == "two");
}

TEST_CASE("latest delivery keeps only the final product from an invocation") {
    zmq_publisher::Channel channel({
        "test", "TOPIC", "inproc://test", zmq_publisher::DeliveryPolicy::latest, 1});

    const auto messages = channel.accept({text("one"), text("two")});

    REQUIRE(messages.size() == 1);
    REQUIRE(messages.front().products.size() == 1);
    CHECK(payload(messages.front().products.front()) == "two");
}

TEST_CASE("sliding window shares retained payload storage") {
    zmq_publisher::Channel channel({
        "test", "TOPIC", "inproc://test", zmq_publisher::DeliveryPolicy::sliding_window, 2});

    const auto first = channel.accept({text("one")});
    const auto first_payload = first.front().products.front().payload();
    channel.accept({text("two")});
    const auto third = channel.accept({text("three")});

    REQUIRE(third.front().products.size() == 2);
    CHECK(payload(third.front().products[0]) == "two");
    CHECK(payload(third.front().products[1]) == "three");
    CHECK(first_payload.use_count() >= 1);
}
