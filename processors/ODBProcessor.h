#ifndef ODBPROCESSOR_H
#define ODBPROCESSOR_H

#include "CommandProcessor.h"

class ODBProcessor : public CommandProcessor {
public:
    ODBProcessor(int verbose = 0, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;

private:
    std::string extractJsonFromOutput(const std::string& commandOutput);
};

#endif // ODBPROCESSOR_H
