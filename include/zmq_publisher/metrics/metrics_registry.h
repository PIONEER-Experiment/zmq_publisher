#pragma once

#include "zmq_publisher/metrics/component_metrics.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace zmq_publisher {

class MetricsRegistry {
public:
    std::shared_ptr<ComponentMetrics> component(const std::string& name);
    std::vector<MetricSnapshot> snapshot() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ComponentMetrics>> components_;
};

}  // namespace zmq_publisher
