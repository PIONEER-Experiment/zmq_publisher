#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace zmq_publisher {

struct MetricSnapshot {
    std::string component;
    std::uint64_t invocations = 0;
    std::uint64_t items = 0;
    std::uint64_t bytes = 0;
    std::uint64_t idle = 0;
    std::uint64_t failures = 0;
    std::uint64_t dropped = 0;
    std::uint64_t blocked_nanoseconds = 0;
    std::uint64_t processing_nanoseconds = 0;
    std::uint64_t latency_nanoseconds = 0;
    std::size_t queue_depth = 0;
    std::size_t queue_high_watermark = 0;
};

}  // namespace zmq_publisher
