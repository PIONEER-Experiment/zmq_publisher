#ifndef PERFORMANCEPROCESSOR_H
#define PERFORMANCEPROCESSOR_H

#include "CommandProcessor.h"
#include "EventProcessor.h"
#include "serializer/Serializer.hh"
#include "MdumpDatabase.h"

class PerformanceProcessor : public CommandProcessor {
public:
    PerformanceProcessor(const std::string& detectorMappingFile, int verbose = 0, bool use_multi_threading = false, int num_workers = 5, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;
    void setCommandRunner(const CommandRunner& runner) override;

private:
    std::string detectorMappingFile;
    EventProcessor eventProcessor;
    unpackers::Serializer* serializer;
    MdumpDatabase mdumpDatabase;
    bool useMultiThreading;
    int numWorkers;
};

#endif // PERFORMANCEPROCESSOR_H
