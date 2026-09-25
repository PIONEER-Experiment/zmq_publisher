#pragma once

#include "zmq_publisher/config/configuration_view.h"

#include <string>
#include <vector>

namespace zmq_publisher::command {

class CommandProcessorConfig {
public:
    static CommandProcessorConfig from_configuration(const ConfigurationView& config);

    const std::vector<std::string>& arguments() const noexcept { return arguments_; }

private:
    explicit CommandProcessorConfig(std::vector<std::string> arguments);

    std::vector<std::string> arguments_;
};

}  // namespace zmq_publisher::command
