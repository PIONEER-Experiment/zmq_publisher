#include "data_transmitter/DataChannelProcessesManager.h"
#include <algorithm> // Include for std::gcd

const int DEFAULT_PROCESSOR_PERIOD = 1000;

DataChannelProcessesManager::DataChannelProcessesManager(size_t bufferSize, int verbose)
    : dataBuffer(bufferSize), verbose(verbose), processorPeriodsGcd(DEFAULT_PROCESSOR_PERIOD) {
}

void DataChannelProcessesManager::addProcessor(GeneralProcessor* processor) {
    processors.push_back(processor);
}

bool DataChannelProcessesManager::runProcesses() {
    bool addedNewData = false;
    for (const auto processor : processors) {
        if (processor->isReadyToProcess()) {
            std::vector<std::string> processedOutput = processor->getProcessedOutput();
            for (const auto& output : processedOutput) {
                addedNewData = true;
                dataBuffer.Push(output);
            }
        }
    }
    return addedNewData;
}

const DataBuffer<std::string>& DataChannelProcessesManager::getDataBuffer() const {
    return dataBuffer;
}

// Update the processorPeriodsGcd member variable
void DataChannelProcessesManager::updateProcessorPeriodsGCD() {
    processorPeriodsGcd = findGCDOfProcessorPeriods();
}

// Get the processorPeriodsGcd value
int DataChannelProcessesManager::getProcessorPeriodsGCD() const {
    return processorPeriodsGcd;
}

int DataChannelProcessesManager::findGCDOfProcessorPeriods() {
    if (processors.empty()) {
        return DEFAULT_PROCESSOR_PERIOD; // Return 1000 if there are no processors
    }

    // Initialize the GCD with the period of the first processor
    int gcd = processors[0]->getPeriod();

    for (size_t i = 1; i < processors.size(); ++i) {
        // Calculate the GCD with the period of the next processor
        gcd = std::gcd(gcd, processors[i]->getPeriod());
    }

    return gcd;
}
