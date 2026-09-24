#include "zmq_publisher/app/application.h"

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

std::size_t output_queue_capacity(const nlohmann::json& configuration) {
    const auto capacity = settings(configuration).value("output-queue-capacity", 1024U);
    if (capacity == 0) {
        throw std::invalid_argument("output-queue-capacity must be greater than zero");
    }
    return capacity;
}

}  // namespace

Application::Application(const nlohmann::json& configuration)
    : worker_count_(worker_count(configuration)),
      plugin_loader_(registry_),
      transport_(output_queue_capacity(configuration)),
      scheduler_(worker_count_),
      channels_(registry_,
                scheduler_,
                transport_,
                ProcessorContext{settings(configuration).value("verbose", 0)}) {
    for (const auto& library : settings(configuration).value(
             "processor-extensions", nlohmann::json::array())) {
        plugin_loader_.load(library.get<std::string>());
    }
    channels_.configure(configuration.value("data-channels", nlohmann::json::object()));
}

int Application::run() {
    SignalHandler signals;
    transport_.start();
    scheduler_.start();
    spdlog::info("ZMQ publisher started with {} channel(s) and {} worker thread(s)",
                 channels_.size(),
                 worker_count_);

    while (!signals.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Stopping ZMQ publisher");
    scheduler_.stop();
    transport_.stop();
    return 0;
}

}  // namespace zmq_publisher
