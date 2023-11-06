#ifndef ODBPROCESSOR_H
#define ODBPROCESSOR_H

#include "CommandProcessor.h"

class ODBProcessor : public CommandProcessor {
public:
    ODBProcessor(int verbose = 0, std::shared_ptr<CommandRunner> runner = nullptr);

    std::vector<std::string> getProcessedOutput() override;

private:
    std::string extractJsonFromOutput(const std::string& commandOutput);
};

#endif // ODBPROCESSOR_H
