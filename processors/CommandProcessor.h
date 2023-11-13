#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "GeneralProcessor.h"
#include "CommandRunner.h"
#include <string>
#include <vector>

class CommandProcessor : public GeneralProcessor {
public:
    CommandProcessor(int verbose = 0, const CommandRunner& runner = CommandRunner(""));
    std::vector<std::string> getProcessedOutput() override;
    void setCommandRunner(const CommandRunner& runner);
    const CommandRunner& getCommandRunner() const;
    bool isReadyToProcess() const override;
    int getPeriod() const override;
    void setPeriod(int newPeriod) override; 
    ~CommandProcessor() override;

protected:
    CommandRunner commandRunner;
};

#endif // COMMAND_PROCESSOR_H
