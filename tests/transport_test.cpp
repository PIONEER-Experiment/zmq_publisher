#include "zmq_publisher/processor/output_message.h"
#include "zmq_publisher/transport/publish_message.h"
#include "zmq_publisher/transport/zmq_transport.h"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <zmq.hpp>

TEST_CASE("transport publishes metadata and unescaped binary product frames") {
    const auto socket_path =
        "/tmp/zmq_publisher_transport_" + std::to_string(::getpid()) + ".sock";
    const auto endpoint = "ipc://" + socket_path;

    zmq_publisher::ZmqTransport transport;
    transport.start();

    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.set(zmq::sockopt::subscribe, "DATA");
    subscriber.set(zmq::sockopt::rcvtimeo, 2000);
    subscriber.connect(endpoint);

    auto publish = [&](std::string topic, std::string value, std::uint64_t sequence) {
        zmq_publisher::PublishMessage message;
        message.endpoint = endpoint;
        message.topic = std::move(topic);
        message.sequence = sequence;
        message.products.push_back(zmq_publisher::OutputMessage::from_text(
            value, "application/x-test", "test-v1"));
        return transport.publish(std::move(message));
    };

    REQUIRE(publish("WARMUP", "warmup", 1) == zmq_publisher::PublishResult::queued);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::atomic<bool> received{false};
    std::thread sender([&] {
        for (int attempt = 0; attempt < 50 && !received.load(); ++attempt) {
            publish("DATA", std::string{"binary\0value", 12}, 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    });

    zmq::message_t topic;
    zmq::message_t metadata;
    zmq::message_t product;
    const auto received_topic = subscriber.recv(topic).has_value();
    received.store(true);
    sender.join();
    REQUIRE(received_topic);
    REQUIRE(subscriber.recv(metadata).has_value());
    REQUIRE(subscriber.recv(product).has_value());

    CHECK(topic.to_string() == "DATA");
    const auto envelope = nlohmann::json::parse(metadata.to_string());
    CHECK(envelope.at("protocol") == "zmq-publisher-v1");
    CHECK(envelope.at("sequence") == 2);
    REQUIRE(envelope.at("products").size() == 1);
    CHECK(envelope.at("products").front().at("content-type") == "application/x-test");
    CHECK(envelope.at("products").front().at("size") == 12);
    CHECK(product.size() == 12);
    CHECK(std::string(static_cast<const char*>(product.data()), product.size()) ==
          std::string{"binary\0value", 12});

    subscriber.close();
    context.shutdown();
    context.close();
    transport.stop();
    std::remove(socket_path.c_str());
}
