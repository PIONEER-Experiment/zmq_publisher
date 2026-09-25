#pragma once

#include <cstddef>

namespace zmq_publisher {

struct ProcessorCapabilities {
    bool thread_safe = false;
    bool preserve_output_order = true;
    std::size_t max_concurrency = 1;
};

}  // namespace zmq_publisher
