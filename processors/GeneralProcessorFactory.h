#ifndef GENERAL_PROCESSOR_FACTORY_H
#define GENERAL_PROCESSOR_FACTORY_H

#include <string>
#include <unordered_map>
#include <functional>
#include "GeneralProcessor.h"

class GeneralProcessorFactory {
public:
    static GeneralProcessorFactory& Instance();

    void RegisterProcessor(const std::string& processorType, std::function<GeneralProcessor*()> creator);
    GeneralProcessor* CreateProcessor(const std::string& processorType) const;

private:
    GeneralProcessorFactory();

private:
    std::unordered_map<std::string, std::function<GeneralProcessor*()>> creators;
};

#endif
