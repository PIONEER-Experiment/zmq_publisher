#include "CommandProcessor.h"
#include "ProjectPrinter.h"

CommandProcessor::CommandProcessor(int verbose, std::shared_ptr<CommandRunner> runner)
    : GeneralProcessor(verbose), commandRunner(runner) {}

std::vector<std::string> CommandProcessor::processOutput() {
    std::vector<std::string> result;

    if (commandRunner) {
        result = commandRunner->execute();
    } else {
        ProjectPrinter printer;
        printer.PrintWarning("CommandRunner is not set for CommandProcessor.", __LINE__, __FILE__);
    }

    return result;
}

void CommandProcessor::setCommandRunner(std::shared_ptr<CommandRunner> runner) {
    commandRunner = runner;
}

std::shared_ptr<CommandRunner> CommandProcessor::getCommandRunner() const {
    return commandRunner;
}

CommandProcessor::~CommandProcessor() {
    // Destructor
}
