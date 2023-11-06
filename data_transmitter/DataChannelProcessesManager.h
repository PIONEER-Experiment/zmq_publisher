#ifndef DATACHANNELPROCESSESMANAGER_H
#define DATACHANNELPROCESSESMANAGER_H

#include <vector>
#include <memory>
#include "GeneralProcessor.h" // Include your GeneralProcessor header here
#include "DataBuffer.h" // Include your DataBuffer header here

class DataChannelProcessesManager {
public:
    DataChannelProcessesManager(size_t bufferSize = 10, int verbose = 0);

    // Add a method to add GeneralProcessor instances to the manager
    void addProcessor(std::shared_ptr<GeneralProcessor> processor);

    // Run all registered processors and add their output to the data buffer
    bool runProcesses();

    // Get the data buffer
    const DataBuffer<std::string>& getDataBuffer() const;

private:
    std::vector<std::shared_ptr<GeneralProcessor>> processors;
    DataBuffer<std::string> dataBuffer;
    int verbose;
};

#endif // DATACHANNELPROCESSESMANAGER_H
