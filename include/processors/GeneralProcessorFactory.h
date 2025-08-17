#ifndef GENERAL_PROCESSOR_FACTORY_H
#define GENERAL_PROCESSOR_FACTORY_H

#include <string>
#include <unordered_map>
#include <functional>
#include "GeneralProcessor.h"

/**
 * @brief A factory class for creating instances of GeneralProcessor.
 *
 * The `GeneralProcessorFactory` class provides functionality to register and create instances
 * of the GeneralProcessor class using a factory pattern.
 */
class GeneralProcessorFactory {
public:
    /**
     * @brief Gets the singleton instance of GeneralProcessorFactory.
     * @return Reference to the singleton instance.
     */
    static GeneralProcessorFactory& Instance();

    /**
     * @brief Registers a processor type with its creator function.
     * @param processorType The type identifier for the processor.
     * @param creator The function that creates an instance of the processor.
     */
    void RegisterProcessor(const std::string& processorType, std::function<GeneralProcessor*()> creator);

    /**
     * @brief Creates an instance of GeneralProcessor based on the provided processor type.
     * @param processorType The type identifier for the processor.
     * @return Pointer to the created GeneralProcessor instance.
     */
    GeneralProcessor* CreateProcessor(const std::string& processorType) const;

private:
    /**
     * @brief Private constructor for GeneralProcessorFactory.
     */
    GeneralProcessorFactory();

private:
    std::unordered_map<std::string, std::function<GeneralProcessor*()>> creators; ///< Map storing processor type and creator functions.
};

#endif // GENERAL_PROCESSOR_FACTORY_H
