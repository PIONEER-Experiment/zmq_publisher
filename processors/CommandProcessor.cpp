#include "CommandProcessor.h"
#include "ProjectPrinter.h"

CommandProcessor::CommandProcessor(int verbose, std::shared_ptr<CommandRunner> runner)
    : GeneralProcessor(verbose), commandRunner(runner) {}

std::vector<std::string> CommandProcessor::getProcessedOutput() {
    std::vector<std::string> result;

    if (commandRunner) {
        result.push_back(commandRunner->execute());
        return result;
    }
    ProjectPrinter printer;
    printer.PrintWarning("CommandRunner is not set for CommandProcessor.", __LINE__, __FILE__);
    return result;
}

void CommandProcessor::setCommandRunner(std::shared_ptr<CommandRunner> runner) {
    commandRunner = runner;
}

std::shared_ptr<CommandRunner> CommandProcessor::getCommandRunner() const {
    return commandRunner;
}

bool CommandProcessor::isReadyToProcess() const {
    if (commandRunner) {
        return commandRunner->isReadyForExecution();
    }
    ProjectPrinter printer;
    printer.PrintWarning("CommandRunner is not set for CommandProcessor.", __LINE__, __FILE__);
    return false;
}

int CommandProcessor::getPeriod() const {
    if (commandRunner) {
        return commandRunner->getWaitTime();
    }
    ProjectPrinter printer;
    printer.PrintWarning("CommandRunner is not set for CommandProcessor.", __LINE__, __FILE__);
    return period;
}

void CommandProcessor::setPeriod(int newPeriod) {
    if (commandRunner) {
        commandRunner->setWaitTime(newPeriod);
        return;
    }
    ProjectPrinter printer;
    printer.PrintWarning("CommandRunner is not set for CommandProcessor.", __LINE__, __FILE__);
    period = newPeriod;
}

CommandProcessor::~CommandProcessor() {
    // Destructor
}
