#pragma once

#include "zmq_publisher/processor/output_message.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace zmq_publisher {

struct PublishMessage {
    std::string endpoint;
    std::string topic;
    std::uint64_t sequence = 0;
    std::chrono::steady_clock::time_point created_at = std::chrono::steady_clock::now();
    std::vector<OutputMessage> products;
};

}  // namespace zmq_publisher
