// CommandRunner.h
#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <string>
#include <vector>
#include <chrono>

/**
 * @brief A utility class for executing commands and managing execution parameters.
 *
 * The `CommandRunner` class provides functionality to execute commands with
 * optional arguments and manage execution parameters such as wait time.
 */
class CommandRunner {
public:
    /**
     * @brief Constructor for CommandRunner with a single command.
     * @param command The command to execute.
     */
    CommandRunner(const std::string& command);

    /**
     * @brief Constructor for CommandRunner with a command and arguments.
     * @param commandWithArgs The command and its arguments as a vector of strings.
     */
    CommandRunner(const std::vector<std::string>& commandWithArgs);

    /**
     * @brief Adds an argument to the command.
     * @param arg The argument to add.
     */
    void addArgument(const std::string& arg);

    /**
     * @brief Sets the wait time in milliseconds between command executions.
     * @param milliseconds The wait time in milliseconds.
     */
    void setWaitTime(int milliseconds);

    /**
     * @brief Executes the command and returns the output.
     * @return The output of the executed command.
     */
    std::string execute();

    /**
     * @brief Checks if the CommandRunner is ready for execution based on the wait time.
     * @return True if ready for execution, false otherwise.
     */
    bool isReadyForExecution() const;

    /**
     * @brief Gets the original command as a string.
     * @return The original command.
     */
    std::string getCommand() const;

    /**
     * @brief Gets the wait time between command executions.
     * @return The wait time in milliseconds.
     */
    int getWaitTime() const;

protected:
    std::vector<std::string> commandWithArgs_; ///< The command and its arguments.
    int waitTime_; ///< The wait time between command executions.
    std::chrono::time_point<std::chrono::high_resolution_clock> lastExecutionTime; ///< Timestamp of the last execution.
};

#endif // COMMANDRUNNER_H
