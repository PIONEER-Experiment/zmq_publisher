#ifndef RUNNUMBERPROCESSOR_H
#define RUNNUMBERPROCESSOR_H

#include "CommandProcessor.h"

class RunNumberProcessor : public CommandProcessor {
public:
    RunNumberProcessor(int verbose = 0, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;

private:
    int extractNumberFromOutput(const std::string& commandOutput);
};

#endif // RUNNUMBERPROCESSOR_H
