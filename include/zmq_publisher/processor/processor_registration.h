#pragma once

#include "zmq_publisher/config/configuration_view.h"
#include "zmq_publisher/processor/processor.h"
#include "zmq_publisher/processor/processor_capabilities.h"
#include "zmq_publisher/processor/processor_context.h"

#include <functional>
#include <memory>

namespace zmq_publisher {

using ProcessorFactory = std::function<std::unique_ptr<Processor>(
    const ConfigurationView&, ProcessorContext&)>;

struct ProcessorRegistration {
    ProcessorFactory factory;
    ProcessorCapabilities capabilities;
};

}  // namespace zmq_publisher
