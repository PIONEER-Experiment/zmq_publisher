#include "CRProcessor.h"
#include "ProjectPrinter.h"
#include <iostream>

CRProcessor::CRProcessor(const std::string& detectorMappingFile, int verbose, std::shared_ptr<CommandRunner> runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile) {
    // Constructor initializes the detector mapping file
}

std::vector<std::string> CRProcessor::processOutput() {
    ProjectPrinter printer;

    // Process the command output here using detectorMappingFile

    if (verbose > 1) {
        printer.PrintWarning("CRProcessor is not implemented for processing command output", __LINE__, __FILE__);
    }

    // Placeholder result
    std::vector<std::string> result;
    result.push_back("CRProcessor output");

    return result;
}
