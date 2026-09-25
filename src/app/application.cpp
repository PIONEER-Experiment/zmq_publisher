#include "zmq_publisher/app/application.h"

#include "zmq_publisher/metrics/metrics_reporter.h"
#include "zmq_publisher/processor/processor_context.h"
#include "zmq_publisher/utility/signal_handler.h"

#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>

namespace zmq_publisher {

namespace {

const nlohmann::json& settings(const nlohmann::json& configuration) {
    static const auto empty = nlohmann::json::object();
    const auto found = configuration.find("general-settings");
    return found == configuration.end() ? empty : *found;
}

std::size_t worker_count(const nlohmann::json& configuration) {
    const auto fallback = std::max(1U, std::thread::hardware_concurrency());
    const auto count = settings(configuration).value("worker-threads", fallback);
    if (count == 0) {
        throw std::invalid_argument("worker-threads must be greater than zero");
    }
    return count;
}

TransportConfig transport_config(const nlohmann::json& configuration) {
    const auto& general = settings(configuration);
    const auto& config = general.contains("transport")
        ? general.at("transport")
        : nlohmann::json::object();

    TransportConfig result;
    result.worker_threads = config.value("worker-threads", 1U);
    result.zmq_io_threads = config.value("zmq-io-threads", 1U);
    result.queue_capacity = config.value("queue-capacity", 1024U);
    const auto policy = config.value("queue-policy", "block");
    if (policy == "block") {
        result.queue_policy = QueuePolicy::block;
    } else if (policy == "drop-newest") {
        result.queue_policy = QueuePolicy::drop_newest;
    } else if (policy == "drop-oldest") {
        result.queue_policy = QueuePolicy::drop_oldest;
    } else if (policy == "fail") {
        result.queue_policy = QueuePolicy::fail;
    } else {
        throw std::invalid_argument("Unknown transport queue policy: " + policy);
    }
    return result;
}

std::unique_ptr<MetricsReporter> metrics_reporter(
    const nlohmann::json& configuration,
    MetricsRegistry& registry) {
    const auto& general = settings(configuration);
    if (!general.contains("telemetry")) {
        return nullptr;
    }
    const auto& config = general.at("telemetry");
    if (!config.value("enabled", true)) {
        return nullptr;
    }
    return std::make_unique<MetricsReporter>(
        registry,
        std::chrono::milliseconds(config.value("interval-ms", 1000)),
        config.value("endpoint", std::string{}),
        config.value("topic", std::string{"METRICS"}));
}

}  // namespace

Application::Application(const nlohmann::json& configuration)
    : worker_count_(worker_count(configuration)),
      metrics_reporter_(metrics_reporter(configuration, metrics_)),
      plugin_loader_(registry_),
      transport_(transport_config(configuration), &metrics_),
      scheduler_(worker_count_, &metrics_),
      channels_(registry_,
                scheduler_,
                transport_,
                ProcessorContext{settings(configuration).value("verbose", 0)},
                &metrics_) {
    for (const auto& library : settings(configuration).value(
             "processor-extensions", nlohmann::json::array())) {
        plugin_loader_.load(library.get<std::string>());
    }
    channels_.configure(configuration.value("data-channels", nlohmann::json::object()));
}

Application::~Application() = default;

int Application::run() {
    SignalHandler signals;
    transport_.start();
    if (metrics_reporter_) {
        metrics_reporter_->start();
    }
    scheduler_.start();
    spdlog::info("ZMQ publisher started with {} channel(s) and {} worker thread(s)",
                 channels_.size(),
                 worker_count_);

    while (!signals.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Stopping ZMQ publisher");
    scheduler_.stop();
    if (metrics_reporter_) {
        metrics_reporter_->stop();
    }
    transport_.stop();
    return 0;
}

}  // namespace zmq_publisher
