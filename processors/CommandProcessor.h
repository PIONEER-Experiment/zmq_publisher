#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "GeneralProcessor.h"
#include "CommandRunner.h"
#include <string>
#include <vector>
#include <memory>  // Include for shared_ptr

class CommandProcessor : public GeneralProcessor {
public:
    CommandProcessor(int verbose = 0, std::shared_ptr<CommandRunner> runner = nullptr);
    virtual std::vector<std::string> processOutput();
    void setCommandRunner(std::shared_ptr<CommandRunner> runner);
    std::shared_ptr<CommandRunner> getCommandRunner() const;
    ~CommandProcessor() override;

private:
    std::shared_ptr<CommandRunner> commandRunner;
};

#endif // COMMAND_PROCESSOR_H
