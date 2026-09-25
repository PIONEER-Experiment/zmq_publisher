#pragma once

#include "zmq_publisher/scheduler/processor_schedule.h"

#include <nlohmann/json.hpp>
#include <string>

namespace zmq_publisher {

class ProcessorDefinition {
public:
    static ProcessorDefinition from_json(
        const nlohmann::json& value,
        std::string location);

    const std::string& type() const noexcept { return type_; }
    const std::string& name() const noexcept { return name_; }
    const ProcessorSchedule& schedule() const noexcept { return schedule_; }
    const nlohmann::json& configuration() const noexcept { return configuration_; }

private:
    ProcessorDefinition(
        std::string type,
        std::string name,
        ProcessorSchedule schedule,
        nlohmann::json configuration);

    std::string type_;
    std::string name_;
    ProcessorSchedule schedule_;
    nlohmann::json configuration_;
};

}  // namespace zmq_publisher
