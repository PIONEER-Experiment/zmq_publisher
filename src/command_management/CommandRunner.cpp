#include "CommandRunner.h"
#include <stdexcept>
#include <array>
#include <memory>
#include <cstdio>
#include <sstream>
#include <chrono>

CommandRunner::CommandRunner(const std::string& command)
    : commandWithArgs_{command}, waitTime_{0} {}

CommandRunner::CommandRunner(const std::vector<std::string>& commandWithArgs)
    : commandWithArgs_(commandWithArgs), waitTime_{0} {}

void CommandRunner::addArgument(const std::string& arg) {
    commandWithArgs_.push_back(arg);
}

void CommandRunner::setWaitTime(int milliseconds) {
    waitTime_ = milliseconds;
}

std::string CommandRunner::execute() {
    std::string output;
    std::array<char, 128> buffer;

    // Build the command string from the vector of strings
    std::string command;
    for (const std::string& arg : commandWithArgs_) {
        command += arg;
        command += ' ';
    }

    // Open a pipe to the command
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("Failed to run the command.");
    }

    // Read the command's output into the string
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            output += buffer.data();
        }
    }

    lastExecutionTime = std::chrono::high_resolution_clock::now();
    return output;
}

bool CommandRunner::isReadyForExecution() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return (currentTime - lastExecutionTime) >= std::chrono::milliseconds(waitTime_);
}

std::string CommandRunner::getCommand() const {
    // Build the command string from the vector of strings
    std::string command;
    for (const std::string& arg : commandWithArgs_) {
        command += arg;
        command += ' ';
    }
    return command;
}

int CommandRunner::getWaitTime() const {
    return waitTime_;
}
