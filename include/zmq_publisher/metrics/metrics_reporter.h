#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace zmq_publisher {

class MetricsRegistry;

class MetricsReporter {
public:
    MetricsReporter(
        MetricsRegistry& registry,
        std::chrono::milliseconds interval,
        std::string endpoint = {},
        std::string topic = "METRICS");
    ~MetricsReporter();

    MetricsReporter(const MetricsReporter&) = delete;
    MetricsReporter& operator=(const MetricsReporter&) = delete;

    void start();
    void stop();

private:
    void run();

    MetricsRegistry& registry_;
    std::chrono::milliseconds interval_;
    std::string endpoint_;
    std::string topic_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

}  // namespace zmq_publisher
