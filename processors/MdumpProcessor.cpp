#include "MdumpProcessor.h"
#include "ProjectPrinter.h"
#include "MdumpPackage.h"
#include "MidasEvent.h"
#include "MidasBank.h"
#include <iostream>

MdumpProcessor::MdumpProcessor(const std::string& detectorMappingFile, int verbose, const CommandRunner& runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile), eventProcessor(detectorMappingFile, verbose) {
}

std::vector<std::string> MdumpProcessor::getProcessedOutput() {
    ProjectPrinter printer;

    // Placeholder result
    std::vector<std::string> result;

    MdumpPackage mdumpPackage(commandRunner.execute());
    for (const MidasEvent& event : mdumpPackage.getEvents()) {
        if (eventProcessor.processEvent(event, "CR00") == 0) {
            // Serialize the event data with EventProcessor and store it in serializedData
            std::string serializedData = eventProcessor.getSerializedData();
            result.push_back(serializedData);
        }
    }

    return result;
}
