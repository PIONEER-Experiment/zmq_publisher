#include "zmq_publisher/extensions/midas/midas_processor.h"

#include "zmq_publisher/processor/processor_registry.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {

zmq_publisher::ProcessorRegistration registration() {
    return {
        [](const nlohmann::json& processor_config, zmq_publisher::ProcessorContext&) {
            const nlohmann::json* config = nullptr;
            if (processor_config.contains("config")) {
                config = &processor_config.at("config");
            } else if (processor_config.contains("midas_receiver_config")) {
                config = &processor_config.at("midas_receiver_config");
            } else {
                throw std::invalid_argument(
                    "MIDAS processor requires 'config' or legacy 'midas_receiver_config'");
            }
            if (!config->is_object()) {
                throw std::invalid_argument("MIDAS processor config must be an object");
            }
            return std::make_unique<zmq_publisher::midas::MidasProcessor>(*config);
        },
        zmq_publisher::ProcessorCapabilities{false, true, 1},
    };
}

}  // namespace

extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry) {
    registry.register_processor("midas", registration());
    registry.register_processor("MidasEventProcessor", registration());
}
