#include "zmq_publisher/metrics/metrics_registry.h"

#include <catch2/catch_test_macros.hpp>
#include <chrono>

TEST_CASE("component metrics retain rate-testing counters") {
    zmq_publisher::MetricsRegistry registry;
    auto metrics = registry.component("processor/test");

    metrics->record_invocation();
    metrics->record_items(3);
    metrics->record_bytes(4096);
    metrics->record_idle();
    metrics->record_failure();
    metrics->record_drop(2);
    metrics->record_blocked(std::chrono::microseconds(4));
    metrics->record_processing(std::chrono::microseconds(7));
    metrics->record_latency(std::chrono::microseconds(9));
    metrics->set_queue_depth(5);
    metrics->set_queue_depth(2);

    const auto snapshots = registry.snapshot();
    REQUIRE(snapshots.size() == 1);
    const auto& snapshot = snapshots.front();
    CHECK(snapshot.component == "processor/test");
    CHECK(snapshot.invocations == 1);
    CHECK(snapshot.items == 3);
    CHECK(snapshot.bytes == 4096);
    CHECK(snapshot.idle == 1);
    CHECK(snapshot.failures == 1);
    CHECK(snapshot.dropped == 2);
    CHECK(snapshot.blocked_nanoseconds == 4000);
    CHECK(snapshot.processing_nanoseconds == 7000);
    CHECK(snapshot.latency_nanoseconds == 9000);
    CHECK(snapshot.queue_depth == 2);
    CHECK(snapshot.queue_high_watermark == 5);
}
