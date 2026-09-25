#include "zmq_publisher/metrics/metrics_reporter.h"

#include "zmq_publisher/metrics/metrics_registry.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>
#include <zmq.hpp>

namespace zmq_publisher {

MetricsReporter::MetricsReporter(
    MetricsRegistry& registry,
    std::chrono::milliseconds interval,
    std::string endpoint,
    std::string topic)
    : registry_(registry),
      interval_(interval),
      endpoint_(std::move(endpoint)),
      topic_(std::move(topic)) {
    if (interval_.count() <= 0) {
        throw std::invalid_argument("Metrics interval must be greater than zero");
    }
}

MetricsReporter::~MetricsReporter() {
    stop();
}

void MetricsReporter::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&MetricsReporter::run, this);
}

void MetricsReporter::stop() {
    running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void MetricsReporter::run() {
    zmq::context_t context(1);
    std::unique_ptr<zmq::socket_t> socket;
    if (!endpoint_.empty()) {
        socket = std::make_unique<zmq::socket_t>(context, zmq::socket_type::pub);
        socket->bind(endpoint_);
    }

    while (running_.load()) {
        const auto started = std::chrono::steady_clock::now();
        nlohmann::json document;
        document["components"] = nlohmann::json::array();
        for (const auto& metric : registry_.snapshot()) {
            document["components"].push_back({
                {"component", metric.component},
                {"invocations", metric.invocations},
                {"items", metric.items},
                {"bytes", metric.bytes},
                {"idle", metric.idle},
                {"failures", metric.failures},
                {"dropped", metric.dropped},
                {"blocked-ns", metric.blocked_nanoseconds},
                {"processing-ns", metric.processing_nanoseconds},
                {"latency-ns", metric.latency_nanoseconds},
                {"queue-depth", metric.queue_depth},
                {"queue-high-watermark", metric.queue_high_watermark},
            });
        }
        const auto payload = document.dump();
        spdlog::info("telemetry {}", payload);
        if (socket) {
            socket->send(zmq::buffer(topic_), zmq::send_flags::sndmore);
            socket->send(zmq::buffer(payload), zmq::send_flags::none);
        }

        while (running_.load() && std::chrono::steady_clock::now() - started < interval_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (socket) {
        socket->close();
    }
    context.shutdown();
    context.close();
}

}  // namespace zmq_publisher
