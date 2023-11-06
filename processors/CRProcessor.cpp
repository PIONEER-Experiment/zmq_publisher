#include "CRProcessor.h"
#include "ProjectPrinter.h"
#include "MdumpPackage.h"
#include "MidasEvent.h"
#include "MidasBank.h"
#include <iostream>

CRProcessor::CRProcessor(const std::string& detectorMappingFile, int verbose, std::shared_ptr<CommandRunner> runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile) {
    eventProcessor = std::make_shared<EventProcessor>(detectorMappingFile, verbose);
}

std::vector<std::string> CRProcessor::getProcessedOutput() {
    ProjectPrinter printer;

    // Placeholder result
    std::vector<std::string> result;

    MdumpPackage mdumpPackage(commandRunner->execute());
    for (const MidasEvent& event : mdumpPackage.getEvents()) {
        if (eventProcessor->processEvent(event, "CR00") == 0) {
            // Serialize the event data with EventProcessor and store it in serializedData
            std::string serializedData = eventProcessor->getSerializedData();
            result.push_back(serializedData);
        }
    }
    

    return result;
}
