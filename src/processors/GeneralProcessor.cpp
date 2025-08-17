// GeneralProcessor.cpp
#include "processors/GeneralProcessor.h"

GeneralProcessor::GeneralProcessor(int verbose) : verbose(verbose) {}

std::vector<std::string> GeneralProcessor::getProcessedOutput() {
    // Default implementation just returns empty list
    std::vector<std::string> result;

    return result;
}

bool GeneralProcessor::isReadyToProcess() const {
    return true; // Always ready to process by default
}

void GeneralProcessor::setVerbose(int verboseLevel) {
    verbose = verboseLevel;
}

int GeneralProcessor::getPeriod() const {
    return period;
}

void GeneralProcessor::setPeriod(int newPeriod) {
    period = newPeriod;
}

GeneralProcessor::~GeneralProcessor() {
    // Destructor
}
