#include "zmq_publisher/transport/zmq_transport.h"

#include "detail/transport_worker.h"
#include "zmq_publisher/metrics/component_metrics.h"
#include "zmq_publisher/metrics/metrics_registry.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace zmq_publisher {

ZmqTransport::ZmqTransport(TransportConfig config, MetricsRegistry* metrics)
    : config_(config), metrics_(metrics) {
    if (config_.worker_threads == 0 || config_.zmq_io_threads == 0 ||
        config_.queue_capacity == 0) {
        throw std::invalid_argument("Transport thread counts and queue capacity must be nonzero");
    }
}

ZmqTransport::~ZmqTransport() {
    stop();
}

void ZmqTransport::start() {
    std::lock_guard lock(lifecycle_mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    workers_.reserve(config_.worker_threads);
    for (std::size_t id = 0; id < config_.worker_threads; ++id) {
        const auto name = "transport/worker-" + std::to_string(id);
        auto component = metrics_
            ? metrics_->component(name)
            : std::make_shared<ComponentMetrics>(name);
        auto worker = std::make_unique<TransportWorker>(
            id,
            config_.queue_capacity,
            config_.queue_policy,
            config_.zmq_io_threads,
            std::move(component));
        worker->start();
        workers_.push_back(std::move(worker));
    }
}

void ZmqTransport::stop() {
    std::vector<std::unique_ptr<TransportWorker>> workers;
    {
        std::lock_guard lock(lifecycle_mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
        workers.swap(workers_);
    }
    for (auto& worker : workers) {
        worker->stop();
    }
}

PublishResult ZmqTransport::publish(PublishMessage message) {
    std::lock_guard lock(lifecycle_mutex_);
    if (!running_ || workers_.empty()) {
        return PublishResult::stopped;
    }
    const auto index = std::hash<std::string>{}(message.endpoint) % workers_.size();
    return workers_[index]->publish(std::move(message));
}

}  // namespace zmq_publisher
