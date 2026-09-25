#include "zmq_publisher/processor/processor_registry.h"

#include <stdexcept>
#include <mutex>
#include <utility>

namespace zmq_publisher {

void ProcessorRegistry::register_processor(
    std::string type,
    ProcessorRegistration registration) {
    if (type.empty()) {
        throw std::invalid_argument("Processor type cannot be empty");
    }
    if (!registration.factory) {
        throw std::invalid_argument("Processor factory cannot be empty: " + type);
    }
    if (registration.capabilities.max_concurrency == 0) {
        throw std::invalid_argument("Processor max_concurrency must be at least one: " + type);
    }

    std::unique_lock lock(mutex_);
    const auto [_, inserted] = registrations_.emplace(
        std::move(type), std::move(registration));
    if (!inserted) {
        throw std::runtime_error("Processor type is already registered");
    }
}

std::unique_ptr<Processor> ProcessorRegistry::create(
    const std::string& type,
    const ConfigurationView& configuration,
    ProcessorContext& context) const {
    ProcessorFactory factory;
    {
        std::shared_lock lock(mutex_);
        const auto found = registrations_.find(type);
        if (found == registrations_.end()) {
            throw std::runtime_error("Unknown processor type: " + type);
        }
        factory = found->second.factory;
    }
    auto processor = factory(configuration, context);
    if (!processor) {
        throw std::runtime_error("Processor factory returned null: " + type);
    }
    return processor;
}

ProcessorCapabilities ProcessorRegistry::capabilities(const std::string& type) const {
    std::shared_lock lock(mutex_);
    const auto found = registrations_.find(type);
    if (found == registrations_.end()) {
        throw std::runtime_error("Unknown processor type: " + type);
    }
    return found->second.capabilities;
}

bool ProcessorRegistry::contains(const std::string& type) const {
    std::shared_lock lock(mutex_);
    return registrations_.find(type) != registrations_.end();
}

}  // namespace zmq_publisher
