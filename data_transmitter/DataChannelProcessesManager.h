#ifndef DATACHANNELPROCESSESMANAGER_H
#define DATACHANNELPROCESSESMANAGER_H

#include <vector>
#include <memory>
#include "GeneralProcessor.h"
#include "DataBuffer.h"

class DataChannelProcessesManager {
public:
    DataChannelProcessesManager(size_t bufferSize = 10, int verbose = 0);

    // Add a method to add GeneralProcessor instances to the manager using raw pointers
    void addProcessor(GeneralProcessor* processor);

    // Run all registered processors and add their output to the data buffer
    bool runProcesses();

    // Get the data buffer
    const DataBuffer<std::string>& getDataBuffer() const;

    // Update the processorPeriodsGcd member variable
    void updateProcessorPeriodsGCD();

    // Get the processorPeriodsGcd value
    int getProcessorPeriodsGCD() const;

private:
    std::vector<GeneralProcessor*> processors;
    DataBuffer<std::string> dataBuffer;
    int verbose;
    int processorPeriodsGcd; // Added member variable for storing GCD

    int findGCDOfProcessorPeriods();
};

#endif // DATACHANNELPROCESSESMANAGER_H
