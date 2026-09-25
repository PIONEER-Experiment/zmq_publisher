#include "zmq_publisher/channel/channel.h"
#include "zmq_publisher/processor/output_message.h"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kPayloadBytes = 1024 * 1024;
constexpr std::size_t kIterations = 2000;

zmq_publisher::OutputMessage payload() {
    return zmq_publisher::OutputMessage(
        zmq_publisher::ByteBuffer(kPayloadBytes, std::byte{0x5a}),
        "application/octet-stream",
        "benchmark-waveform-v1");
}

void run(std::string name, zmq_publisher::DeliveryPolicy policy, std::size_t size) {
    zmq_publisher::Channel channel({name, "BENCH", "inproc://benchmark", policy, size});
    std::size_t published = 0;
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < kIterations; ++index) {
        published += channel.accept({payload()}).size();
    }
    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    const auto gib = static_cast<double>(kIterations * kPayloadBytes) /
        (1024.0 * 1024.0 * 1024.0);
    std::cout << std::left << std::setw(18) << name
              << " input_gib_per_s=" << std::fixed << std::setprecision(2) << gib / elapsed
              << " messages_per_s=" << static_cast<double>(published) / elapsed << '\n';
}

}  // namespace

int main() {
    run("each", zmq_publisher::DeliveryPolicy::each, 1);
    run("batch-16", zmq_publisher::DeliveryPolicy::batch, 16);
    run("sliding-16", zmq_publisher::DeliveryPolicy::sliding_window, 16);
}
