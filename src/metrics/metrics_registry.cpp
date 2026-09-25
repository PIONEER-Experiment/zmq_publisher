#include "zmq_publisher/metrics/metrics_registry.h"

namespace zmq_publisher {

std::shared_ptr<ComponentMetrics> MetricsRegistry::component(const std::string& name) {
    std::lock_guard lock(mutex_);
    const auto found = components_.find(name);
    if (found != components_.end()) {
        return found->second;
    }
    auto metrics = std::make_shared<ComponentMetrics>(name);
    components_.emplace(name, metrics);
    return metrics;
}

std::vector<MetricSnapshot> MetricsRegistry::snapshot() const {
    std::vector<std::shared_ptr<ComponentMetrics>> components;
    {
        std::lock_guard lock(mutex_);
        components.reserve(components_.size());
        for (const auto& [_, metrics] : components_) {
            components.push_back(metrics);
        }
    }

    std::vector<MetricSnapshot> result;
    result.reserve(components.size());
    for (const auto& metrics : components) {
        result.push_back(metrics->snapshot());
    }
    return result;
}

}  // namespace zmq_publisher
