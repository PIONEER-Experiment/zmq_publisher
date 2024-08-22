#include "PerformanceProcessor.h"
#include "ProjectPrinter.h"
#include "MdumpPackage.h"
#include "MidasEvent.h"
#include "MidasBank.h"
#include <iostream>
#include <chrono>

PerformanceProcessor::PerformanceProcessor(const std::string& detectorMappingFile, int verbose, bool use_multi_threading, int num_workers, const CommandRunner& runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile), 
    eventProcessor(detectorMappingFile, verbose), 
    serializer(new unpackers::Serializer(detectorMappingFile, 0, 0, 0)), 
    mdumpDatabase(runner,verbose,num_workers),
    useMultiThreading(use_multi_threading) {
}


std::vector<std::string> PerformanceProcessor::getProcessedOutput() {
    // Placeholder result
    std::vector<std::string> result;

    //Get mdump output
    MdumpPackage mdumpPackage;
    if (useMultiThreading) {
        mdumpDatabase.startWorker();
        if (!mdumpDatabase.hasNewData()) {
            return result;
        }
        mdumpPackage = MdumpPackage(mdumpDatabase.getLatestMdumpOutput());
    } else {
        mdumpPackage = MdumpPackage(commandRunner.execute());
    }

    //Process CC bank events within mdump package
    for (const MidasEvent& event : mdumpPackage.getEvents()) {
        if (eventProcessor.processPerformanceDataBank(event, "CC") == 0) {
            std::vector<std::string> serializedData = eventProcessor.getSerializedData();
            result.insert(result.end(), serializedData.begin(), serializedData.end());
        }
        
    }

    return result;
}

void PerformanceProcessor::setCommandRunner(const CommandRunner& runner) {
    commandRunner = runner;
    mdumpDatabase.setCommandRunner(runner);
}