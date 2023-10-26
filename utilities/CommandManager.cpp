#include "CommandManager.h"
#include <stdexcept>
#include <array>
#include <memory>
#include <cstdio>
#include <sstream>
#include <chrono>

CommandManager::CommandManager(const std::string& command)
    : commandWithArgs_{command}, waitTime_{0} {}

CommandManager::CommandManager(const std::vector<std::string>& commandWithArgs)
    : commandWithArgs_(commandWithArgs), waitTime_{0} {}

void CommandManager::addArgument(const std::string& arg) {
    commandWithArgs_.push_back(arg);
}

void CommandManager::setWaitTime(int milliseconds) {
    waitTime_ = milliseconds;
}

std::string CommandManager::execute() {
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

bool CommandManager::isReadyForExecution() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return (currentTime - lastExecutionTime) >= std::chrono::milliseconds(waitTime_);
}

std::string CommandManager::getCommand() const {
    // Build the command string from the vector of strings
    std::string command;
    for (const std::string& arg : commandWithArgs_) {
        command += arg;
        command += ' ';
    }
    return command;
}

int CommandManager::getWaitTime() const {
    return waitTime_;
}
