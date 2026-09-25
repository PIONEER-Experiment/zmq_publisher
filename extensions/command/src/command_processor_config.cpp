#include "zmq_publisher/extensions/command/command_processor_config.h"

#include <stdexcept>
#include <utility>

namespace zmq_publisher::command {

CommandProcessorConfig::CommandProcessorConfig(std::vector<std::string> arguments)
    : arguments_(std::move(arguments)) {}

CommandProcessorConfig CommandProcessorConfig::from_configuration(
    const ConfigurationView& config) {
    config.validate_keys({"command"});
    auto arguments = config.required<std::vector<std::string>>("command");
    if (arguments.empty() || arguments.front().empty()) {
        throw std::invalid_argument("Command processor command cannot be empty");
    }
    return CommandProcessorConfig(std::move(arguments));
}

}  // namespace zmq_publisher::command
