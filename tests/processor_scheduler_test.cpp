#include "zmq_publisher/scheduler/processor_scheduler.h"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {

class CountingProcessor final : public zmq_publisher::Processor {
public:
    std::vector<std::string> process() override {
        return {std::to_string(++calls_)};
    }

private:
    std::atomic<int> calls_{0};
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
        [&](std::vector<std::string> output) {
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
