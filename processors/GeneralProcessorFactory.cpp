#include "GeneralProcessorFactory.h"

GeneralProcessorFactory::GeneralProcessorFactory() {
    // Constructor (if needed)
}

GeneralProcessorFactory& GeneralProcessorFactory::Instance() {
    static GeneralProcessorFactory factory;
    return factory;
}

void GeneralProcessorFactory::RegisterProcessor(const std::string& processorType, std::shared_ptr<GeneralProcessor> processor) {
    processors[processorType] = processor;
}

std::shared_ptr<GeneralProcessor> GeneralProcessorFactory::CreateProcessor(const std::string& processorType) const {
    auto it = processors.find(processorType);
    if (it != processors.end()) {
        return it->second;
    }
    return nullptr;
}
