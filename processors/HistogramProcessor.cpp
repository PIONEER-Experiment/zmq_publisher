#include "HistogramProcessor.h"
#include "HistogramStorage.h"
#include <chrono>
#include <ctime>

HistogramProcessor::HistogramProcessor(int verbose) : GeneralProcessor(verbose) {
    // Initialize last publish time to the current time
    lastPublishTime = std::chrono::system_clock::now();
}

std::vector<std::string> HistogramProcessor::getProcessedOutput() {
    std::vector<std::string> result;
    result.push_back(HistogramStorage::getInstance().serialize());

    // Update the last publish time after processing
    lastPublishTime = std::chrono::system_clock::now();

    return result;
}

bool HistogramProcessor::isReadyToProcess() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return (currentTime - lastPublishTime) >= std::chrono::milliseconds(getPeriod());
}

int HistogramProcessor::getPeriod() const {
    return GeneralProcessor::getPeriod();
}

void HistogramProcessor::setPeriod(int newPeriod) {
    GeneralProcessor::setPeriod(newPeriod);
}

HistogramProcessor::~HistogramProcessor() {
    // Destructor implementation
}
