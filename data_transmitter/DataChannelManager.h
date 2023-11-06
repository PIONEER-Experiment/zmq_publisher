#ifndef DATA_CHANNEL_MANAGER_H
#define DATA_CHANNEL_MANAGER_H

#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "DataChannel.h"

class DataChannelManager {
public:
    DataChannelManager(const nlohmann::json& channelConfig, int verbose = 0);

    DataChannel* getChannel(const std::string& channelId);
    const std::map<std::string, DataChannel>& getChannelMap() const;
    std::vector<DataChannel> getAllChannels() const;
    void addChannel(const std::string& channelId, const std::string& name, int publishesPerBatch, int publishesIgnoredAfterBatch);
    void addChannel(const std::string& channelId, const nlohmann::json& channelConfig);
    bool removeChannel(const std::string& channelId);

private:
    std::map<std::string, DataChannel> channels;
    int tickTime;
    int verbose;
};

#endif
