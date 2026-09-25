#include "zmq_publisher/scheduler/processor_scheduler.h"

#include "zmq_publisher/metrics/component_metrics.h"
#include "zmq_publisher/metrics/metrics_registry.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>

namespace zmq_publisher {

namespace {
constexpr auto kNotReadyRetryInterval = std::chrono::milliseconds(10);
}

struct ProcessorScheduler::Entry {
    std::string name;
    std::unique_ptr<Processor> processor;
    ProcessorSchedule schedule;
    ProcessorCapabilities capabilities;
    OutputHandler output_handler;
    std::shared_ptr<ComponentMetrics> metrics;
    std::chrono::steady_clock::time_point next_run;
    std::size_t in_flight = 0;
};

ProcessorScheduler::ProcessorScheduler(std::size_t worker_count, MetricsRegistry* metrics)
    : worker_count_(worker_count), metrics_(metrics) {
    if (worker_count == 0) {
        throw std::invalid_argument("ProcessorScheduler requires at least one worker");
    }
}

ProcessorScheduler::~ProcessorScheduler() {
    stop();
}

void ProcessorScheduler::add_processor(
    std::string name,
    std::unique_ptr<Processor> processor,
    ProcessorSchedule schedule,
    ProcessorCapabilities capabilities,
    OutputHandler output_handler) {
    if (!processor) {
        throw std::invalid_argument("Cannot schedule a null processor");
    }
    if (!output_handler) {
        throw std::invalid_argument("Processor output handler cannot be empty");
    }
    if (schedule.interval.count() < 0) {
        throw std::invalid_argument("Processor interval cannot be negative");
    }
    if (schedule.mode != ScheduleMode::continuous && schedule.interval.count() == 0) {
        throw std::invalid_argument("A periodic processor interval must be greater than zero");
    }
    if (!capabilities.thread_safe || capabilities.preserve_output_order) {
        capabilities.max_concurrency = 1;
    }

    std::lock_guard lock(mutex_);
    if (running_) {
        throw std::logic_error("Processors must be added before the scheduler starts");
    }
    auto component = metrics_
        ? metrics_->component("processor/" + name)
        : std::make_shared<ComponentMetrics>("processor/" + name);
    entries_.push_back(std::make_unique<Entry>(Entry{
        std::move(name),
        std::move(processor),
        schedule,
        capabilities,
        std::move(output_handler),
        std::move(component),
        std::chrono::steady_clock::now(),
        0,
    }));
}

void ProcessorScheduler::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    const auto now = std::chrono::steady_clock::now();
    for (auto& entry : entries_) {
        entry->next_run = now;
    }
    workers_.reserve(worker_count_);
    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back(&ProcessorScheduler::worker_loop, this);
    }
    timer_thread_ = std::thread(&ProcessorScheduler::timer_loop, this);
}

void ProcessorScheduler::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!running_ && !timer_thread_.joinable() && workers_.empty()) {
            return;
        }
        running_ = false;
    }
    timer_cv_.notify_all();
    worker_cv_.notify_all();

    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    ready_queue_.clear();
}

void ProcessorScheduler::timer_loop() {
    std::unique_lock lock(mutex_);
    while (running_) {
        const auto now = std::chrono::steady_clock::now();
        auto next_deadline = std::chrono::steady_clock::time_point::max();
        bool queued_work = false;

        for (std::size_t index = 0; index < entries_.size(); ++index) {
            auto& entry = *entries_[index];
            if (entry.in_flight >= entry.capabilities.max_concurrency) {
                continue;
            }
            if (entry.next_run <= now) {
                const auto dispatch_count = entry.schedule.mode == ScheduleMode::continuous
                    ? entry.capabilities.max_concurrency - entry.in_flight
                    : std::size_t{1};
                entry.in_flight += dispatch_count;
                for (std::size_t dispatch = 0; dispatch < dispatch_count; ++dispatch) {
                    ready_queue_.push_back(index);
                }
                queued_work = true;

                if (entry.schedule.mode == ScheduleMode::fixed_rate) {
                    do {
                        entry.next_run += entry.schedule.interval;
                    } while (entry.next_run <= now);
                } else {
                    entry.next_run = std::chrono::steady_clock::time_point::max();
                }
            } else {
                next_deadline = std::min(next_deadline, entry.next_run);
            }
        }

        if (queued_work) {
            worker_cv_.notify_all();
        }
        if (!running_) {
            break;
        }
        if (next_deadline == std::chrono::steady_clock::time_point::max()) {
            timer_cv_.wait(lock);
        } else {
            timer_cv_.wait_until(lock, next_deadline);
        }
    }
}

void ProcessorScheduler::worker_loop() {
    while (true) {
        std::size_t index = 0;
        Entry* entry = nullptr;
        {
            std::unique_lock lock(mutex_);
            worker_cv_.wait(lock, [this] { return !running_ || !ready_queue_.empty(); });
            if (ready_queue_.empty()) {
                if (!running_) {
                    return;
                }
                continue;
            }
            index = ready_queue_.front();
            ready_queue_.pop_front();
            entry = entries_[index].get();
        }

        bool was_ready = false;
        auto status = ProcessStatus::idle;
        const auto processing_started = std::chrono::steady_clock::now();
        entry->metrics->record_invocation();
        try {
            was_ready = entry->processor->is_ready();
            if (was_ready) {
                auto result = entry->processor->process();
                status = result.status();
                if (status == ProcessStatus::produced_output) {
                    std::size_t bytes = 0;
                    for (const auto& message : result.output()) {
                        bytes += message.size();
                    }
                    entry->metrics->record_items(result.output().size());
                    entry->metrics->record_bytes(bytes);
                    entry->output_handler(std::move(result.output()));
                } else if (status == ProcessStatus::idle) {
                    entry->metrics->record_idle();
                }
            } else {
                entry->metrics->record_idle();
            }
        } catch (const std::exception& error) {
            entry->metrics->record_failure();
            spdlog::error("Processor '{}' failed: {}", entry->name, error.what());
        } catch (...) {
            entry->metrics->record_failure();
            spdlog::error("Processor '{}' failed with an unknown exception", entry->name);
        }
        entry->metrics->record_processing(std::chrono::steady_clock::now() - processing_started);

        {
            std::lock_guard lock(mutex_);
            --entry->in_flight;
            if (status == ProcessStatus::stopped) {
                entry->next_run = std::chrono::steady_clock::time_point::max();
                continue;
            }
            if (entry->schedule.mode != ScheduleMode::fixed_rate) {
                auto interval = entry->schedule.interval;
                if (entry->schedule.mode == ScheduleMode::continuous) {
                    interval = status == ProcessStatus::idle
                        ? entry->schedule.idle_backoff
                        : std::chrono::microseconds::zero();
                } else if (!was_ready || status == ProcessStatus::idle) {
                    interval = std::max(
                        interval,
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            kNotReadyRetryInterval));
                }
                entry->next_run = std::chrono::steady_clock::now() + interval;
            }
        }
        timer_cv_.notify_one();
    }
}

}  // namespace zmq_publisher
