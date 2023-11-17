#include "RunNumberProcessor.h"
#include "ProjectPrinter.h"
#include "HistogramStorage.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <chrono>

RunNumberProcessor::RunNumberProcessor(int verbose, const CommandRunner& runner)
: CommandProcessor(verbose, runner) {
    // Additional initialization specific to RunNumberProcessor, if needed
}

std::vector<std::string> RunNumberProcessor::getProcessedOutput() {
    std::vector<std::string> result;

    // Execute the odbedit command
    std::string odbOutput = commandRunner.execute();

    //Extract value to int
    int extractedValue = extractNumberFromOutput(odbOutput);
    
    if (extractedValue != -1) {
        HistogramStorage::getInstance().setRunNumber(extractedValue);
    }

    return result;
}


int RunNumberProcessor::extractNumberFromOutput(const std::string& commandOutput) {
    // Remove leading and trailing whitespaces, if any
    std::string trimmedOutput = commandOutput;
    size_t start = commandOutput.find_first_not_of(" \t\n");
    size_t end = commandOutput.find_last_not_of(" \t\n");
    
    if (start != std::string::npos && end != std::string::npos) {
        trimmedOutput = commandOutput.substr(start, end - start + 1);
    } else {
        ProjectPrinter printer;
        printer.PrintWarning("Failed to extract run number. Empty or whitespace-only output.", __LINE__, __FILE__);
        return -1;
    }

    // Convert the trimmed string to an integer
    try {
        return std::atoi(trimmedOutput.c_str());
    } catch (const std::exception& e) {
        ProjectPrinter printer;
        printer.PrintWarning("Failed to extract run number. Conversion to integer failed.", __LINE__, __FILE__);
        return -1; 
    }
}
