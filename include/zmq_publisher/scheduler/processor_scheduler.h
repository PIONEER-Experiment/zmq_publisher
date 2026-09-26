#pragma once

#include "zmq_publisher/processor/processor.h"
#include "zmq_publisher/processor/processor_capabilities.h"
#include "zmq_publisher/processor/output_message.h"
#include "zmq_publisher/scheduler/processor_schedule.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace zmq_publisher {

class MetricsRegistry;

class ProcessorScheduler {
public:
    using OutputHandler = std::function<void(std::vector<OutputMessage>)>;

    explicit ProcessorScheduler(std::size_t worker_count, MetricsRegistry* metrics = nullptr);
    ~ProcessorScheduler();

    ProcessorScheduler(const ProcessorScheduler&) = delete;
    ProcessorScheduler& operator=(const ProcessorScheduler&) = delete;

    void add_processor(
        std::string name,
        std::unique_ptr<Processor> processor,
        ProcessorSchedule schedule,
        ProcessorCapabilities capabilities,
        OutputHandler output_handler);
    void start();
    void stop();

private:
    struct Entry;

    void wake_processor(std::size_t index);
    bool dispatch_event_driven_locked(std::size_t index);
    void timer_loop();
    void worker_loop();

    std::size_t worker_count_;
    MetricsRegistry* metrics_;
    bool running_ = false;
    std::mutex mutex_;
    std::condition_variable timer_cv_;
    std::condition_variable worker_cv_;
    std::vector<std::unique_ptr<Entry>> entries_;
    std::deque<std::size_t> ready_queue_;
    std::thread timer_thread_;
    std::vector<std::thread> workers_;
};

}  // namespace zmq_publisher
