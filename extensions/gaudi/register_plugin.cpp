#include "zmq_publisher/extensions/gaudi/gaudi_processor.h"
#include "zmq_publisher/extensions/gaudi/gaudi_processor_config.h"
#include "zmq_publisher/processor/processor_registry.h"

#include <memory>

extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry) {
    registry.register_processor("midas-gaudi", {
        [](const zmq_publisher::ConfigurationView& config,
           zmq_publisher::ProcessorContext&) {
            return std::make_unique<zmq_publisher::gaudi::GaudiProcessor>(
                zmq_publisher::gaudi::GaudiProcessorConfig::from_configuration(config));
        },
        zmq_publisher::ProcessorCapabilities{false, true, 1},
    });
}
