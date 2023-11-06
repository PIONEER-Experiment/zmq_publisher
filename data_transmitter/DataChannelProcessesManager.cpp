#include "DataChannelProcessesManager.h"
#include <algorithm> // Include for std::gcd

DataChannelProcessesManager::DataChannelProcessesManager(size_t bufferSize, int verbose)
    : dataBuffer(bufferSize), verbose(verbose) {
}

void DataChannelProcessesManager::addProcessor(std::shared_ptr<GeneralProcessor> processor) {
    processors.push_back(processor);
}

bool DataChannelProcessesManager::runProcesses() {
    bool addedNewData = false;
    for (const auto& processor : processors) {
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
