#include "DataChannelManager.h"
const std::string DEFAULT_NAME = "";
const int DEFAULT_PUBLISHES_PER_BATCH = 1;
const int DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH = 0;

DataChannelManager::DataChannelManager(const nlohmann::json& channelConfig) {
    for (auto it = channelConfig.begin(); it != channelConfig.end(); ++it) {
        const std::string& channelId = it.key();
        const nlohmann::json& channelData = it.value();

        addChannel(channelId,channelData);
    }
}

DataChannel* DataChannelManager::getChannel(const std::string& channelId) {
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        return &it->second;
    }
    return nullptr; // Channel not found
}

const std::map<std::string, DataChannel>& DataChannelManager::getChannelMap() const {
    return channels;
}

std::vector<DataChannel> DataChannelManager::getAllChannels() const {
    std::vector<DataChannel> channelObjects;
    for (const auto& pair : channels) {
        channelObjects.push_back(pair.second);
    }
    return channelObjects;
}

void DataChannelManager::addChannel(const std::string& channelId, const std::string& name, int publishesPerBatch, int publishesIgnoredAfterBatch) {
    // Create a new DataChannel and add it to the manager
    DataChannel dataChannel(name, publishesPerBatch, publishesIgnoredAfterBatch);
    channels[channelId] = dataChannel;
}

void DataChannelManager::addChannel(const std::string& channelId, const nlohmann::json& channelConfig) {
    std::string name = DEFAULT_NAME;
    int publishesPerBatch = DEFAULT_PUBLISHES_PER_BATCH;
    int publishesIgnoredAfterBatch = DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH;

    if (channelConfig.contains("name")) {
        name = channelConfig["name"];
    }
    if (channelConfig.contains("publishes-per-batch")) {
        publishesPerBatch = channelConfig["publishes-per-batch"];
    }
    if (channelConfig.contains("publishes-ignored-after-batch")) {
        publishesIgnoredAfterBatch = channelConfig["publishes-ignored-after-batch"];
    }

    // Create a new DataChannel and add it to the manager
    DataChannel dataChannel(name, publishesPerBatch, publishesIgnoredAfterBatch);
    channels[channelId] = dataChannel;
}


bool DataChannelManager::removeChannel(const std::string& channelId) {
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        channels.erase(it);
        return true; // Channel removed successfully
    }
    return false; // Channel not found
}



