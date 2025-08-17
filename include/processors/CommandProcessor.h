// CommandProcessor.h
#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "processors/GeneralProcessor.h"
#include "command_management/CommandRunner.h"
#include <string>
#include <vector>

/**
 * @brief A processor that executes commands and processes their output.
 *
 * The `CommandProcessor` class is a specialization of `GeneralProcessor` that executes
 * commands using a `CommandRunner` and processes the command output.
 */
class CommandProcessor : public GeneralProcessor {
public:
    /**
     * @brief Constructor for CommandProcessor.
     * @param verbose The verbosity level for logging (default is 0).
     * @param runner The command runner responsible for executing commands (default is an empty runner).
     */
    CommandProcessor(int verbose = 0, const CommandRunner& runner = CommandRunner(""));

    /**
     * @brief Destructor for CommandProcessor.
     */
    ~CommandProcessor() override;

    /**
     * @brief Gets the processed output after executing the command.
     * @return Vector of strings representing the processed output.
     */
    std::vector<std::string> getProcessedOutput() override;

    /**
     * @brief Sets the command runner for executing commands.
     * @param runner The command runner to set.
     * @details The command runner class gets automatically generated and set based on the config.
     * @see DataChannelManager::addChannel
     */
    virtual void setCommandRunner(const CommandRunner& runner);

    /**
     * @brief Gets the current command runner.
     * @return Reference to the current command runner.
     */
    const CommandRunner& getCommandRunner() const;

    /**
     * @brief Checks if the processor is ready to process.
     * @return True if ready to process, false otherwise.
     */
    bool isReadyToProcess() const override;

    /**
     * @brief Gets the processing period for the CommandProcessor.
     * @return The processing period.
     */
    int getPeriod() const override;

    /**
     * @brief Sets the processing period for the CommandProcessor.
     * @param newPeriod The new processing period.
     */
    void setPeriod(int newPeriod) override;

protected:
    CommandRunner commandRunner; ///< The command runner responsible for executing commands.
};

#endif // COMMAND_PROCESSOR_H
