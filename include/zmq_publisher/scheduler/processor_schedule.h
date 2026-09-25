#pragma once

#include <chrono>

namespace zmq_publisher {

enum class ScheduleMode {
    fixed_delay,
    fixed_rate,
    continuous,
};

struct ProcessorSchedule {
    ScheduleMode mode = ScheduleMode::fixed_delay;
    std::chrono::microseconds interval{std::chrono::seconds(1)};
    std::chrono::microseconds idle_backoff{std::chrono::milliseconds(10)};
};

}  // namespace zmq_publisher
