// DataChannel.h
#ifndef DATA_CHANNEL_H
#define DATA_CHANNEL_H

#include <string>
#include <memory>
#include "data_transmitter/DataChannelProcessesManager.h"

// Forward declarations to avoid circular imports
class DataTransmitter;
class DataTransmitterManager;

/**
 * @brief Represents a data channel for publishing events.
 *
 * The `DataChannel` class manages the publication of events for a specific channel.
 * It tracks the number of events published, events seen, and provides functionality
 * for handling breaks in event publication.
 */
class DataChannel {
public:
    /**
     * @brief Default constructor for DataChannel.
     */
    DataChannel();

    /**
     * @brief Constructor for DataChannel with specified attributes.
     * @param name The name of the data channel.
     * @param eventsBeforeBreak The number of events before taking a break.
     * @param eventsToIgnoreInBreak The number of events to ignore during a break.
     */
    DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak);

    /**
     * @brief Constructor for DataChannel with specified attributes and address.
     * @param name The name of the data channel.
     * @param eventsBeforeBreak The number of events before taking a break.
     * @param eventsToIgnoreInBreak The number of events to ignore during a break.
     * @param address The address associated with the data channel.
     */
    DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak, const std::string& address);

    /**
     * @brief Publishes events for the data channel.
     * @return True if successful, false otherwise.
     */
    bool publish();

    /**
     * @brief Sets the name of the data channel.
     * @param name The name to set.
     */
    void setName(const std::string& name);

    /**
     * @brief Sets the number of events before taking a break.
     * @param eventsBeforeBreak The number of events before taking a break.
     */
    void setEventsBeforeBreak(int eventsBeforeBreak);

    /**
     * @brief Sets the number of events to ignore during a break.
     * @param eventsToIgnoreInBreak The number of events to ignore during a break.
     */
    void setEventsToIgnoreInBreak(int eventsToIgnoreInBreak);

    /**
     * @brief Sets the address associated with the data channel.
     * @param address The address to set.
     */
    void setAddress(const std::string& address);

    /**
     * @brief Gets the name of the data channel.
     * @return The name of the data channel.
     */
    const std::string& getName() const;

    /**
     * @brief Gets the number of events before taking a break.
     * @return The number of events before taking a break.
     */
    int getEventsBeforeBreak() const;

    /**
     * @brief Gets the number of events to ignore during a break.
     * @return The number of events to ignore during a break.
     */
    int getEventsToIgnoreInBreak() const;

    /**
     * @brief Gets the address associated with the data channel.
     * @return The address associated with the data channel.
     */
    const std::string& getAddress() const;

    /**
     * @brief Gets the number of events published.
     * @return The number of events published.
     */
    int getEventsPublished() const;

    /**
     * @brief Gets the number of events seen.
     * @return The number of events seen.
     */
    int getEventsSeen() const;

    /**
     * @brief Checks if the data channel is on break.
     * @return True if on break, false otherwise.
     */
    bool isOnBreak() const;

    /**
     * @brief Gets the number of events seen during a break.
     * @return The number of events seen during a break.
     */
    int getEventsSeenOnBreak() const;

    /**
     * @brief Gets the tick time for the data channel.
     * @return The tick time in milliseconds.
     */
    int getTickTime() const;

    /**
     * @brief Increments the count of events published.
     */
    void published();

    /**
     * @brief Increments the count of events seen.
     */
    void seen();

    /**
     * @brief Resets the data channel attributes.
     */
    void reset();

    /**
     * @brief Prints the attributes of the data channel.
     */
    void printAttributes() const;

    /**
     * @brief Sets the DataChannelProcessesManager for the data channel.
     * @param manager The DataChannelProcessesManager to set.
     */
    void setDataChannelProcessesManager(DataChannelProcessesManager manager);

    /**
     * @brief Adds a GeneralProcessor to the DataChannelProcessesManager.
     * @param processor Pointer to the GeneralProcessor to add.
     */
    void addProcessToManager(GeneralProcessor* processor);

    /**
     * @brief Updates the tick time for the data channel.
     */
    void updateTickTime();

private:
    std::string name; ///< Name of the data channel.
    int eventsBeforeBreak; ///< Number of events before taking a break.
    int eventsToIgnoreInBreak; ///< Number of events to ignore during a break.
    std::string address; ///< Address associated with the data channel.
    int eventsPublished; ///< Number of events published.
    int eventsSeen; ///< Number of events seen.
    bool onBreak; ///< Flag indicating if the data channel is on break.
    int eventsSeenOnBreak; ///< Number of events seen during a break.
    std::shared_ptr<DataTransmitter> transmitter; ///< DataTransmitter for publishing events.
    DataChannelProcessesManager processesManager; ///< Manager for data channel processes.
    int tickTime; ///< Tick time for the data channel.

    /**
     * @brief Checks if a break should be taken based on the configured criteria in \ref config.json.
     * @return True if a break should be taken, false otherwise.
     */
    bool shouldTakeBreak();

    /**
     * @brief Initiates a break for the data channel.
     */
    void startBreak();

    /**
     * @brief Initializes the DataTransmitter for the data channel.
     */
    void initializeTransmitter();
};

#endif // DATA_CHANNEL_H
