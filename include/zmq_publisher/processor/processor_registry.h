#pragma once

#include "zmq_publisher/processor/processor_registration.h"

#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace zmq_publisher {

class ProcessorRegistry {
public:
    void register_processor(std::string type, ProcessorRegistration registration);
    std::unique_ptr<Processor> create(
        const std::string& type,
        const ConfigurationView& configuration,
        ProcessorContext& context) const;
    ProcessorCapabilities capabilities(const std::string& type) const;
    bool contains(const std::string& type) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, ProcessorRegistration> registrations_;
};

}  // namespace zmq_publisher
