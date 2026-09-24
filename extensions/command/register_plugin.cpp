#include "zmq_publisher/extensions/command/command_processor.h"

#include "zmq_publisher/processor/processor_registry.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<std::string> parse_command(const nlohmann::json& processor_config) {
    const auto& config = processor_config.contains("config")
        ? processor_config.at("config")
        : processor_config;
    if (!config.contains("command")) {
        throw std::invalid_argument("Command processor requires 'command'");
    }

    const auto& command = config.at("command");
    if (command.is_array()) {
        return command.get<std::vector<std::string>>();
    }
    if (command.is_string()) {
        return {"/bin/sh", "-c", command.get<std::string>()};
    }
    throw std::invalid_argument("Command must be a string or an array of arguments");
}

zmq_publisher::ProcessorRegistration registration() {
    return {
        [](const nlohmann::json& config, zmq_publisher::ProcessorContext&) {
            return std::make_unique<zmq_publisher::command::CommandProcessor>(
                parse_command(config));
        },
        zmq_publisher::ProcessorCapabilities{false, true, 1},
    };
}

}  // namespace

extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry) {
    registry.register_processor("command", registration());
    registry.register_processor("CommandProcessor", registration());
}
