#include "processors/CommandProcessor.h"

CommandProcessor::CommandProcessor(int verbose, const CommandRunner& runner)
    : GeneralProcessor(verbose), commandRunner(runner) {}

std::vector<std::string> CommandProcessor::getProcessedOutput() {
    std::vector<std::string> result;
    result.push_back(commandRunner.execute());
    return result;
}

void CommandProcessor::setCommandRunner(const CommandRunner& runner) {
    commandRunner = runner;
}

const CommandRunner& CommandProcessor::getCommandRunner() const {
    return commandRunner;
}

bool CommandProcessor::isReadyToProcess() const {
    if (commandRunner.isReadyForExecution()) {
        return true;
    }
    return false;
}

int CommandProcessor::getPeriod() const {
    return commandRunner.getWaitTime();
}

void CommandProcessor::setPeriod(int newPeriod) {
    commandRunner.setWaitTime(newPeriod);
}

CommandProcessor::~CommandProcessor() {
    // Destructor
}
