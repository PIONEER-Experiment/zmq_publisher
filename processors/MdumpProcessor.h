#ifndef MDUMPPROCESSOR_H
#define MDUMPPROCESSOR_H

#include "CommandProcessor.h"
#include "EventProcessor.h"

class MdumpProcessor : public CommandProcessor {
public:
    MdumpProcessor(const std::string& detectorMappingFile, int verbose = 0, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;

private:
    std::string detectorMappingFile;
    EventProcessor eventProcessor;
};

#endif // MDUMPPROCESSOR_H
