#include "zmq_publisher/extensions/gaudi/gaudi_client.h"
#include "zmq_publisher/extensions/gaudi/gaudi_processor_config.h"
#include "zmq_publisher/config/configuration_view.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <future>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace {

struct RequestHeader {
    std::array<char, 8> magic;
    std::uint64_t request_id;
    std::uint64_t event_size;
    std::uint64_t flags;
};

struct ResponseHeader {
    std::array<char, 8> magic;
    std::uint64_t request_id;
    std::uint32_t status;
    std::uint32_t metadata_size;
    std::uint64_t payload_size;
};

void receive_all(int socket, void* destination, std::size_t size) {
    auto* output = static_cast<std::byte*>(destination);
    while (size != 0) {
        const auto received = ::recv(socket, output, size, 0);
        REQUIRE(received > 0);
        output += received;
        size -= static_cast<std::size_t>(received);
    }
}

void send_all(int socket, const void* source, std::size_t size) {
    const auto* input = static_cast<const std::byte*>(source);
    while (size != 0) {
        const auto sent = ::send(socket, input, size, MSG_NOSIGNAL);
        REQUIRE(sent > 0);
        input += sent;
        size -= static_cast<std::size_t>(sent);
    }
}

}  // namespace

TEST_CASE("GaudiClient exchanges each batch event before sending the next") {
    const std::string socket_path =
        "/tmp/zmq-publisher-gaudi-test-" + std::to_string(::getpid()) + ".sock";
    ::unlink(socket_path.c_str());

    std::promise<void> listening;
    auto ready = listening.get_future();
    std::thread server([&] {
        const int listener = ::socket(AF_UNIX, SOCK_STREAM, 0);
        REQUIRE(listener >= 0);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::memcpy(address.sun_path, socket_path.c_str(), socket_path.size() + 1);
        REQUIRE(::bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0);
        REQUIRE(::listen(listener, 1) == 0);
        listening.set_value();

        const int client = ::accept(listener, nullptr, nullptr);
        REQUIRE(client >= 0);
        std::array<std::byte, 4> first_event{};
        std::array<std::byte, 3> second_event{};
        RequestHeader first_request{};
        receive_all(client, &first_request, sizeof(RequestHeader));
        receive_all(client, first_event.data(), first_event.size());
        CHECK(first_request.magic == std::array<char, 8>{'P', 'I', 'G', 'A', '0', '0', '0', '2'});
        CHECK(first_request.request_id == 1);
        CHECK(first_request.event_size == first_event.size());
        CHECK(first_request.flags == 0);

        const std::string acknowledgement = R"({"control":"processed","products":[]})";
        const ResponseHeader ack_response{
            {'P', 'I', 'G', 'O', '0', '0', '0', '2'},
            first_request.request_id,
            0,
            static_cast<std::uint32_t>(acknowledgement.size()),
            0};
        send_all(client, &ack_response, sizeof(ack_response));
        send_all(client, acknowledgement.data(), acknowledgement.size());

        RequestHeader second_request{};
        receive_all(client, &second_request, sizeof(RequestHeader));
        receive_all(client, second_event.data(), second_event.size());
        CHECK(second_request.request_id == 2);
        CHECK(second_request.event_size == second_event.size());
        CHECK(second_request.flags == 1);

        const std::string metadata = R"({"test":true,"products":[{"content-type":"application/json","schema":"dqm-waveform-v1","offset":0,"size":3}]})";
        const std::array<std::byte, 3> payload{
            std::byte{0x10}, std::byte{0x20}, std::byte{0x30}};
        const ResponseHeader response{
            {'P', 'I', 'G', 'O', '0', '0', '0', '2'},
            second_request.request_id,
            0,
            static_cast<std::uint32_t>(metadata.size()),
            payload.size()};
        send_all(client, &response, sizeof(response));
        send_all(client, metadata.data(), metadata.size());
        send_all(client, payload.data(), payload.size());
        ::close(client);
        ::close(listener);
    });

    ready.wait();
    const std::array<std::byte, 4> first_event{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    const std::array<std::byte, 3> second_event{
        std::byte{0x05}, std::byte{0x06}, std::byte{0x07}};
    zmq_publisher::gaudi::GaudiClient client(
        socket_path, std::chrono::milliseconds(1000), std::chrono::milliseconds(1000));
    const auto responses = client.process_batch({
        {first_event.data(), first_event.size(), false},
        {second_event.data(), second_event.size(), true},
    });
    REQUIRE(responses.size() == 2);
    CHECK(responses[0].metadata == R"({"control":"processed","products":[]})");
    CHECK(responses[0].products.empty());
    CHECK(responses[1].metadata == R"({"test":true,"products":[{"content-type":"application/json","schema":"dqm-waveform-v1","offset":0,"size":3}]})");
    REQUIRE(responses[1].products.size() == 1);
    CHECK(responses[1].products[0].content_type == "application/json");
    CHECK(responses[1].products[0].schema == "dqm-waveform-v1");
    REQUIRE(responses[1].products[0].payload.size() == 3);
    CHECK(responses[1].products[0].payload[0] == std::byte{0x10});
    CHECK(responses[1].products[0].payload[2] == std::byte{0x30});

    server.join();
    ::unlink(socket_path.c_str());
}

TEST_CASE("Gaudi processor configuration keeps MIDAS and sidecar settings separate") {
    const nlohmann::json document{
        {"midas", {
            {"experiment", "ONLINE"},
        }},
        {"processing", {
            {"events-per-request", 16},
            {"maximum-batch-bytes", 8 * 1024 * 1024},
        }},
        {"publication", {
            {"every-events", 5000},
            {"every-ms", 250},
        }},
        {"sidecar", {
            {"socket-path", "/tmp/analysis.sock"},
            {"request-timeout-ms", 45000},
            {"launcher", "/opt/pioneer/run-sidecar"},
            {"job-options", "/opt/pioneer/online.py"},
            {"startup-timeout-ms", 120000},
        }},
    };
    const zmq_publisher::ConfigurationView view(document, "processor.config");
    const auto config =
        zmq_publisher::gaudi::GaudiProcessorConfig::from_configuration(view);
    CHECK(config.experiment() == "ONLINE");
    CHECK(config.events_per_request() == 16);
    CHECK(config.maximum_batch_bytes() == 8 * 1024 * 1024);
    CHECK(config.publish_every_events() == 5000);
    CHECK(config.publish_every_ms() == 250);
    CHECK(config.socket_path() == "/tmp/analysis.sock");
    CHECK(config.request_timeout_ms() == 45000);
    CHECK(config.launcher() == "/opt/pioneer/run-sidecar");
    CHECK(config.job_options() == "/opt/pioneer/online.py");
    CHECK(config.startup_timeout_ms() == 120000);

    const nlohmann::json flat_legacy{
        {"experiment", "ONLINE"},
        {"socket-path", "/tmp/analysis.sock"},
    };
    CHECK_THROWS_AS(
        zmq_publisher::gaudi::GaudiProcessorConfig::from_configuration(
            {flat_legacy, "processor.config"}),
        std::invalid_argument);

    auto incomplete_sidecar = document;
    incomplete_sidecar["sidecar"].erase("job-options");
    CHECK_THROWS_AS(
        zmq_publisher::gaudi::GaudiProcessorConfig::from_configuration(
            {incomplete_sidecar, "processor.config"}),
        std::invalid_argument);
}
