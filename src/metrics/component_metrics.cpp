#include "zmq_publisher/metrics/component_metrics.h"

#include <utility>

namespace zmq_publisher {

ComponentMetrics::ComponentMetrics(std::string component)
    : component_(std::move(component)) {}

void ComponentMetrics::record_invocation() noexcept { ++invocations_; }
void ComponentMetrics::record_items(std::size_t count) noexcept { items_ += count; }
void ComponentMetrics::record_bytes(std::size_t count) noexcept { bytes_ += count; }
void ComponentMetrics::record_idle() noexcept { ++idle_; }
void ComponentMetrics::record_failure() noexcept { ++failures_; }
void ComponentMetrics::record_drop(std::size_t count) noexcept { dropped_ += count; }

void ComponentMetrics::record_blocked(std::chrono::nanoseconds duration) noexcept {
    blocked_nanoseconds_ += static_cast<std::uint64_t>(duration.count());
}

void ComponentMetrics::record_processing(std::chrono::nanoseconds duration) noexcept {
    processing_nanoseconds_ += static_cast<std::uint64_t>(duration.count());
}

void ComponentMetrics::record_latency(std::chrono::nanoseconds duration) noexcept {
    latency_nanoseconds_ += static_cast<std::uint64_t>(duration.count());
}

void ComponentMetrics::set_queue_depth(std::size_t depth) noexcept {
    queue_depth_.store(depth, std::memory_order_relaxed);
    auto high = queue_high_watermark_.load(std::memory_order_relaxed);
    while (depth > high && !queue_high_watermark_.compare_exchange_weak(
                               high, depth, std::memory_order_relaxed)) {
    }
}

MetricSnapshot ComponentMetrics::snapshot() const {
    return {
        component_,
        invocations_.load(std::memory_order_relaxed),
        items_.load(std::memory_order_relaxed),
        bytes_.load(std::memory_order_relaxed),
        idle_.load(std::memory_order_relaxed),
        failures_.load(std::memory_order_relaxed),
        dropped_.load(std::memory_order_relaxed),
        blocked_nanoseconds_.load(std::memory_order_relaxed),
        processing_nanoseconds_.load(std::memory_order_relaxed),
        latency_nanoseconds_.load(std::memory_order_relaxed),
        queue_depth_.load(std::memory_order_relaxed),
        queue_high_watermark_.load(std::memory_order_relaxed),
    };
}

}  // namespace zmq_publisher
