#include "HistogramProcessor.h"

HistogramProcessor::HistogramProcessor(int verbose) : GeneralProcessor(verbose) {
    // Constructor implementation
}

std::vector<std::string> HistogramProcessor::getProcessedOutput() {
    std::vector<std::string> result;
    result.push_back("Testing...");
    return result;
}

bool HistogramProcessor::isReadyToProcess() const {
    return true; 
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

