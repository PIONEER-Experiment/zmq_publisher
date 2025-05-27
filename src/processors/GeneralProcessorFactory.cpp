#include "GeneralProcessorFactory.h"
#include <stdexcept>

GeneralProcessorFactory::GeneralProcessorFactory() {
    // Constructor (if needed)
}

GeneralProcessorFactory& GeneralProcessorFactory::Instance() {
    static GeneralProcessorFactory factory;
    return factory;
}

void GeneralProcessorFactory::RegisterProcessor(const std::string& processorType, std::function<GeneralProcessor*()> creator) {
    creators[processorType] = creator;
}

GeneralProcessor* GeneralProcessorFactory::CreateProcessor(const std::string& processorType) const {
    auto it = creators.find(processorType);
    if (it != creators.end()) {
        return it->second();
    }
    throw std::runtime_error("Processor not found");
}
