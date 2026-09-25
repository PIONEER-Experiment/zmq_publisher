#pragma once

#include "zmq_publisher/transport/queue_policy.h"

#include <cstddef>

namespace zmq_publisher {

struct TransportConfig {
    std::size_t worker_threads = 1;
    std::size_t zmq_io_threads = 1;
    std::size_t queue_capacity = 1024;
    QueuePolicy queue_policy = QueuePolicy::block;
};

}  // namespace zmq_publisher
