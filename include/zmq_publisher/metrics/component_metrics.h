#pragma once

#include "zmq_publisher/metrics/metric_snapshot.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace zmq_publisher {

class ComponentMetrics {
public:
    explicit ComponentMetrics(std::string component);

    void record_invocation() noexcept;
    void record_items(std::size_t count) noexcept;
    void record_bytes(std::size_t count) noexcept;
    void record_idle() noexcept;
    void record_failure() noexcept;
    void record_drop(std::size_t count = 1) noexcept;
    void record_blocked(std::chrono::nanoseconds duration) noexcept;
    void record_processing(std::chrono::nanoseconds duration) noexcept;
    void record_latency(std::chrono::nanoseconds duration) noexcept;
    void set_queue_depth(std::size_t depth) noexcept;

    MetricSnapshot snapshot() const;

private:
    std::string component_;
    std::atomic<std::uint64_t> invocations_{0};
    std::atomic<std::uint64_t> items_{0};
    std::atomic<std::uint64_t> bytes_{0};
    std::atomic<std::uint64_t> idle_{0};
    std::atomic<std::uint64_t> failures_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<std::uint64_t> blocked_nanoseconds_{0};
    std::atomic<std::uint64_t> processing_nanoseconds_{0};
    std::atomic<std::uint64_t> latency_nanoseconds_{0};
    std::atomic<std::size_t> queue_depth_{0};
    std::atomic<std::size_t> queue_high_watermark_{0};
};

}  // namespace zmq_publisher
