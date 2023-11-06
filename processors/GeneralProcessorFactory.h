#ifndef GENERAL_PROCESSOR_FACTORY_H
#define GENERAL_PROCESSOR_FACTORY_H

#include <string>
#include <unordered_map>
#include <memory>
#include "GeneralProcessor.h"

class GeneralProcessorFactory {
public:
    static GeneralProcessorFactory& Instance();

    void RegisterProcessor(const std::string& processorType, std::shared_ptr<GeneralProcessor> processor);
    std::shared_ptr<GeneralProcessor> CreateProcessor(const std::string& processorType) const;

private:
    GeneralProcessorFactory();

private:
    std::unordered_map<std::string, std::shared_ptr<GeneralProcessor>> processors;
};

#endif
