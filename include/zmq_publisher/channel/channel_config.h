#pragma once

#include "zmq_publisher/channel/delivery_policy.h"

#include <cstddef>
#include <string>

namespace zmq_publisher {

struct ChannelConfig {
    std::string id;
    std::string topic;
    std::string endpoint = "tcp://127.0.0.1:5555";
    DeliveryPolicy delivery_policy = DeliveryPolicy::each;
    std::size_t delivery_size = 1;
};

}  // namespace zmq_publisher
