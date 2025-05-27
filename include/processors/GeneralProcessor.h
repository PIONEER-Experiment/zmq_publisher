// GeneralProcessor.h
#ifndef GENERAL_PROCESSOR_H
#define GENERAL_PROCESSOR_H

#include <string>
#include <vector>

/**
 * @brief An abstract base class representing a general processor.
 *
 * The `GeneralProcessor` class defines the common interface for all processor types.
 * @details This class serves as a base for various processor implementations. It provides
 * common functionality such as setting verbosity level, getting processed output, and managing
 * processing periods. Derived classes should override specific methods to implement
 * processor-specific behavior.
 */
class GeneralProcessor {
public:
    /**
     * @brief Constructor for GeneralProcessor.
     * @param verbose The verbosity level for logging (default is 0).
     */
    GeneralProcessor(int verbose = 0);

    /**
     * @brief Virtual destructor for GeneralProcessor.
     */
    virtual ~GeneralProcessor();

    /**
     * @brief Gets the processed output as a vector of strings.
     * @return Vector of strings representing the processed output.
     * @details The vector of strings is added to the circular buffer to be published.
     * So an array of length 2 will add 2 entries to the circular buffer, for example.
     * Empty arrays will not be added to the circular buffer and will therefore not
     * but published.
     * @see DataChannelProcessesManager::runProcesses()
     */
    virtual std::vector<std::string> getProcessedOutput();

    /**
     * @brief Checks if the processor is ready to process.
     * @return True if ready to process, false otherwise.
     * @details getProcessedOutput will not be called unless this is true. By default,
     * this method always returns true. 
     * @see DataChannelProcessesManager::runProcesses() 
     */
    virtual bool isReadyToProcess() const;

    /**
     * @brief Sets the verbosity level for logging.
     * @param verboseLevel The new verbosity level.
     */
    void setVerbose(int verboseLevel);

    /**
     * @brief Gets the processing period.
     * @return The processing period.
     */
    virtual int getPeriod() const;

    /**
     * @brief Sets the processing period.
     * @param newPeriod The new processing period.
     */
    virtual void setPeriod(int newPeriod);

protected:
    int verbose; ///< Verbosity level for logging.
    int period;  ///< Processing period.
};

#endif // GENERAL_PROCESSOR_H
