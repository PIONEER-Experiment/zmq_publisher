#include "GeneralProcessor.h"

GeneralProcessor::GeneralProcessor(int verbose) : verbose(verbose) {
    // Constructor logic, if any
}

std::vector<std::string> GeneralProcessor::processOutput(const std::string& output) {
    // Implement the processing logic here, which can use the verbose member
    std::vector<std::string> result;
    result.push_back(output);

    // You can use the 'verbose' member here in the processing logic

    return result;
}
