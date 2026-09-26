#include "zmq_publisher/scheduler/processor_scheduler.h"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <thread>

namespace {

class CountingProcessor final : public zmq_publisher::Processor {
public:
    zmq_publisher::ProcessResult process() override {
        std::vector<zmq_publisher::OutputMessage> output;
        output.push_back(zmq_publisher::OutputMessage::from_text(std::to_string(++calls_)));
        return zmq_publisher::ProcessResult::produced(std::move(output));
    }

private:
    std::atomic<int> calls_{0};
};

class ConcurrentProcessor final : public zmq_publisher::Processor {
public:
    ConcurrentProcessor(std::atomic<int>& active, std::atomic<int>& maximum)
        : active_(active), maximum_(maximum) {}

    zmq_publisher::ProcessResult process() override {
        const auto active = ++active_;
        auto observed = maximum_.load();
        while (active > observed && !maximum_.compare_exchange_weak(observed, active)) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        --active_;
        return zmq_publisher::ProcessResult::produced(
            {zmq_publisher::OutputMessage::from_text("value")});
    }

private:
    std::atomic<int>& active_;
    std::atomic<int>& maximum_;
};

class WakeableProcessor final : public zmq_publisher::Processor {
public:
    explicit WakeableProcessor(std::atomic<int>& calls) : calls_(calls) {}

    zmq_publisher::ProcessResult process() override {
        ++calls_;
        return zmq_publisher::ProcessResult::idle();
    }

    bool set_wakeup_handler(WakeupHandler handler) override {
        handler_ = std::move(handler);
        return true;
    }

    void wake() { handler_(); }

private:
    std::atomic<int>& calls_;
    WakeupHandler handler_;
};

}  // namespace

TEST_CASE("the scheduler repeatedly dispatches a fixed-delay processor") {
    zmq_publisher::ProcessorScheduler scheduler(2);
    std::mutex mutex;
    std::condition_variable output_ready;
    int outputs = 0;

    scheduler.add_processor(
        "counter",
        std::make_unique<CountingProcessor>(),
        {zmq_publisher::ScheduleMode::fixed_delay, std::chrono::milliseconds(10)},
        {},
        [&](std::vector<zmq_publisher::OutputMessage> output) {
            REQUIRE_FALSE(output.empty());
            {
                std::lock_guard lock(mutex);
                ++outputs;
            }
            output_ready.notify_one();
        });

    scheduler.start();
    {
        std::unique_lock lock(mutex);
        const bool completed = output_ready.wait_for(
            lock, std::chrono::seconds(1), [&] { return outputs >= 3; });
        CHECK(completed);
    }
    scheduler.stop();
}

TEST_CASE("continuous scheduling can redispatch without a fixed delay") {
    zmq_publisher::ProcessorScheduler scheduler(1);
    std::mutex mutex;
    std::condition_variable output_ready;
    int outputs = 0;

    scheduler.add_processor(
        "continuous-counter",
        std::make_unique<CountingProcessor>(),
        {zmq_publisher::ScheduleMode::continuous,
         std::chrono::microseconds::zero(),
         std::chrono::microseconds::zero()},
        {},
        [&](std::vector<zmq_publisher::OutputMessage>) {
            {
                std::lock_guard lock(mutex);
                ++outputs;
            }
            output_ready.notify_one();
        });

    scheduler.start();
    {
        std::unique_lock lock(mutex);
        CHECK(output_ready.wait_for(
            lock, std::chrono::milliseconds(100), [&] { return outputs >= 100; }));
    }
    scheduler.stop();
}

TEST_CASE("thread-safe continuous processors can use declared concurrency") {
    zmq_publisher::ProcessorScheduler scheduler(4);
    std::atomic<int> active{0};
    std::atomic<int> maximum{0};
    std::mutex mutex;
    std::condition_variable output_ready;
    int outputs = 0;

    scheduler.add_processor(
        "parallel",
        std::make_unique<ConcurrentProcessor>(active, maximum),
        {zmq_publisher::ScheduleMode::continuous,
         std::chrono::microseconds::zero(),
         std::chrono::microseconds::zero()},
        {true, false, 4},
        [&](std::vector<zmq_publisher::OutputMessage>) {
            {
                std::lock_guard lock(mutex);
                ++outputs;
            }
            output_ready.notify_one();
        });

    scheduler.start();
    {
        std::unique_lock lock(mutex);
        REQUIRE(output_ready.wait_for(
            lock, std::chrono::seconds(1), [&] { return outputs >= 4; }));
    }
    scheduler.stop();
    CHECK(maximum.load() >= 2);
}

TEST_CASE("event-driven processors run only when their source wakes them") {
    zmq_publisher::ProcessorScheduler scheduler(1);
    std::atomic<int> calls{0};
    auto processor = std::make_unique<WakeableProcessor>(calls);
    auto* wakeable = processor.get();

    scheduler.add_processor(
        "wakeable",
        std::move(processor),
        {zmq_publisher::ScheduleMode::event_driven,
         std::chrono::microseconds::zero(),
         std::chrono::microseconds::zero()},
        {},
        [](std::vector<zmq_publisher::OutputMessage>) {});
    scheduler.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(calls.load() == 0);

    wakeable->wake();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (calls.load() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    scheduler.stop();
    CHECK(calls.load() == 1);
}
