#include "zmq_publisher/extensions/midas/midas_processor.h"
#include "zmq_publisher/extensions/midas/midas_processor_config.h"

#include "zmq_publisher/processor/processor_registry.h"

#include <memory>

namespace {

zmq_publisher::ProcessorRegistration registration() {
    return {
        [](const zmq_publisher::ConfigurationView& config,
           zmq_publisher::ProcessorContext&) {
            const auto typed_config =
                zmq_publisher::midas::MidasProcessorConfig::from_configuration(config);
            return std::make_unique<zmq_publisher::midas::MidasProcessor>(typed_config);
        },
        zmq_publisher::ProcessorCapabilities{false, true, 1},
    };
}

}  // namespace

extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry) {
    registry.register_processor("midas", registration());
}
