// DataChannelManager.h
#ifndef DATA_CHANNEL_MANAGER_H
#define DATA_CHANNEL_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "data_transmitter/DataChannel.h"

/**
 * @brief Manages data channels and their configuration.
 *
 * The `DataChannelManager` class is responsible for managing data channels,
 * providing access to individual channels, and coordinating their publication.
 */
class DataChannelManager {
public:
    /**
     * @brief Constructor for DataChannelManager.
     * @param channelConfig JSON configuration for initializing data channels.
     * @param verbose Verbosity level for logging (default is 0).
     * Though, this is automatically set by \ref config.json
     */
    DataChannelManager(const nlohmann::json& channelConfig, int verbose = 0);

    /**
     * @brief Publishes data for all registered data channels.
     * @return True if successful (though success doesn't necessarily mean data was published), 
     * false otherwise.
     */
    bool publish();

    /**
     * @brief Gets a pointer to a specific data channel by ID.
     * @param channelId The ID of the data channel to retrieve.
     * @return Pointer to the requested DataChannel, or nullptr if not found.
     */
    DataChannel* getChannel(const std::string& channelId);

    /**
     * @brief Gets a map of all registered data channels.
     * @return Map of data channels, where keys are channel IDs and values are DataChannel objects.
     */
    const std::map<std::string, DataChannel>& getChannelMap() const;

    /**
     * @brief Gets a vector of all registered data channels.
     * @return Vector of all DataChannel objects.
     */
    std::vector<DataChannel> getAllChannels() const;

    /**
     * @brief Adds a new data channel to the manager.
     * @param channelId The ID of the data channel to add.
     * @param dataChannel The DataChannel object to add.
     */
    void addChannel(const std::string& channelId, DataChannel dataChannel);

    /**
     * @brief Adds a new data channel to the manager using JSON configuration.
     * @param channelId The ID of the data channel to add.
     * @param channelConfig JSON configuration for the data channel.
     */
    void addChannel(const std::string& channelId, const nlohmann::json& channelConfig);

    /**
     * @brief Removes a data channel from the manager.
     * @param channelId The ID of the data channel to remove.
     * @return True if successful, false otherwise.
     */
    bool removeChannel(const std::string& channelId);

    /**
     * @brief Gets the global tick time for data channel publication.
     * @return The global tick time in milliseconds.
     */
    int getGlobalTickTime() const;

    /**
     * @brief Sets the global tick time for data channel publication.
     */
    void setGlobalTickTime();

    /**
     * @brief Sets a custom global tick time for data channel publication.
     * @param tickTime The custom tick time in milliseconds.
     */
    void setGlobalTickTime(int tickTime);

private:
    std::map<std::string, DataChannel> channels; ///< Map of data channels.
    int globalTickTime; ///< Global tick time for data channel publication.
    int verbose; ///< Verbosity level for logging.

    // Private method for getting a value from JSON with default and warning
    template<typename T>
    T getOrDefault(const nlohmann::json& obj, 
                   const std::string& key, 
                   const T& defaultValue, 
                   const std::string& channelId,
                   const std::string& context = "",
                   bool warnIfMissing = true) 
    {
        if (obj.contains(key)) {
            try {
                return obj.at(key).get<T>();
            } catch (const std::exception& e) {
                spdlog::warn("Failed to parse key '{}' in channel '{}': {} (context: {}). Using default value.", 
                             key, channelId, e.what(), context);
                return defaultValue;
            }
        } else {
            if (warnIfMissing) {
                spdlog::warn("Key '{}' not found in channel '{}' (context: {}). Using default value.", 
                             key, channelId, context);
            }
            return defaultValue;
        }
    }
};

#endif // DATA_CHANNEL_MANAGER_H
