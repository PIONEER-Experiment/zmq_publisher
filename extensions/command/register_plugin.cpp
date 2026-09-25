#include "zmq_publisher/extensions/command/command_processor.h"
#include "zmq_publisher/extensions/command/command_processor_config.h"

#include "zmq_publisher/processor/processor_registry.h"

#include <memory>

namespace {

zmq_publisher::ProcessorRegistration registration() {
    return {
        [](const zmq_publisher::ConfigurationView& config,
           zmq_publisher::ProcessorContext&) {
            const auto typed_config =
                zmq_publisher::command::CommandProcessorConfig::from_configuration(config);
            return std::make_unique<zmq_publisher::command::CommandProcessor>(
                typed_config);
        },
        zmq_publisher::ProcessorCapabilities{false, true, 1},
    };
}

}  // namespace

extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry) {
    registry.register_processor("command", registration());
}
