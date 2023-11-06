#ifndef CRPROCESSOR_H
#define CRPROCESSOR_H

#include "CommandProcessor.h"

class CRProcessor : public CommandProcessor {
public:
    CRProcessor(const std::string& detectorMappingFile, int verbose = 0, std::shared_ptr<CommandRunner> runner = nullptr);

    // Override the processOutput method to handle command output
    std::vector<std::string> processOutput() override;

private:
    std::string detectorMappingFile;
};

#endif // CRPROCESSOR_H
