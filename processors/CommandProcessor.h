#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "GeneralProcessor.h"
#include "CommandRunner.h"
#include <string>
#include <vector>
#include <memory>

class CommandProcessor : public GeneralProcessor {
public:
    CommandProcessor(int verbose = 0, std::shared_ptr<CommandRunner> runner = nullptr);
    std::vector<std::string> getProcessedOutput() override;
    void setCommandRunner(std::shared_ptr<CommandRunner> runner);
    std::shared_ptr<CommandRunner> getCommandRunner() const;
    bool isReadyToProcess() const override;
    int getPeriod() const;
    void setPeriod(int newPeriod); 
    ~CommandProcessor() override;

protected:
    std::shared_ptr<CommandRunner> commandRunner;
};

#endif // COMMAND_PROCESSOR_H
