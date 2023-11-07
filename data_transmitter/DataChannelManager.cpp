#include "DataChannelManager.h"
#include "DataChannelProcessesManager.h"
#include "GeneralProcessorFactory.h"
#include "GeneralProcessor.h"
#include "CommandProcessor.h"
#include "CommandRunner.h"
#include "ProjectPrinter.h"
#include "TypeChecker.h"
#include <algorithm> // Include for std::gcd
#include <iostream>

//Default config
const std::string DEFAULT_NAME                   = "";
const int DEFAULT_PUBLISHES_PER_BATCH            = 1;
const int DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH  = 0;
const int DEFAULT_EVENTS_IN_CIRCULAR_BUFFER      = 1;
const std::string DEFAULT_ZMQ_ADDRESS            = "tcp://127.0.0.1:5555";
const int DEFAULT_PERIOD_MS                      = 1000;
const std::string DEFAULT_COMMAND_STRING         = "";

DataChannelManager::DataChannelManager(const nlohmann::json& channelConfig, int verbose) 
    : verbose(verbose) {
    for (auto it = channelConfig.begin(); it != channelConfig.end(); ++it) {
        const std::string& channelId = it.key();
        const nlohmann::json& channelData = it.value();

        addChannel(channelId,channelData);
    }
}

bool DataChannelManager::publish() {
    bool success = true;

    for (auto& channelPair : channels) {
        if (!channelPair.second.publish()) {
            success = false;
            ProjectPrinter printer;
            printer.PrintWarning("Channel " + channelPair.first + " has failed to publish.", __LINE__, __FILE__);
            channelPair.second.printAttributes();
        }
    }

    return success;
}

DataChannel* DataChannelManager::getChannel(const std::string& channelId) {
    auto it = channels.find(channelId);
    if (it != channels.end()) {
        return &it->second;
    }
    return nullptr; 
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

void DataChannelManager::addChannel(const std::string& channelId, DataChannel dataChannel) {
    channels[channelId] = dataChannel;
}

void DataChannelManager::addChannel(const std::string& channelId, const nlohmann::json& channelConfig) {
    ProjectPrinter printer;
    std::string name = DEFAULT_NAME;
    int publishesPerBatch = DEFAULT_PUBLISHES_PER_BATCH;
    int publishesIgnoredAfterBatch = DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH;
    std::string zmq_address = DEFAULT_ZMQ_ADDRESS;
    int eventsInCircularBuffer = DEFAULT_EVENTS_IN_CIRCULAR_BUFFER;
    GeneralProcessorFactory& factory = GeneralProcessorFactory::Instance();

    if (channelConfig.contains("name")) {
        name = channelConfig["name"].get<std::string>();
    } else {
        printer.PrintWarning("Name not found in channel " + channelId + " configuration. Using the default name: " + DEFAULT_NAME, __LINE__, __FILE__);
    }

    if (channelConfig.contains("publishes-per-batch")) {
        publishesPerBatch = channelConfig["publishes-per-batch"].get<int>();
    } else {
        printer.PrintWarning("Publishes per batch not found in channel " + channelId + " configuration. Using the default value: " + std::to_string(DEFAULT_PUBLISHES_PER_BATCH), __LINE__, __FILE__);
    }

    if (channelConfig.contains("publishes-ignored-after-batch")) {
        publishesIgnoredAfterBatch = channelConfig["publishes-ignored-after-batch"].get<int>();
    } else {
        printer.PrintWarning("Publishes ignored after batch not found in channel " + channelId + " configuration. Using the default value: " + std::to_string(DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH), __LINE__, __FILE__);
    }

    if (channelConfig.contains("zmq-address")) {
        zmq_address = channelConfig["zmq-address"].get<std::string>();
    } else {
        printer.PrintWarning("ZMQ address not found in channel " + channelId + " configuration. Using the default ZMQ address: " + DEFAULT_ZMQ_ADDRESS, __LINE__, __FILE__);
    }

    if (channelConfig.contains("num-events-in-circular-buffer")) {
        eventsInCircularBuffer = channelConfig["num-events-in-circular-buffer"].get<int>();
    } else {
        printer.PrintWarning("Num events in circular buffer not found in channel " + channelId + " configuration. Using the default value: " + std::to_string(DEFAULT_EVENTS_IN_CIRCULAR_BUFFER), __LINE__, __FILE__);
    }

    //Initialize DataChannel (will also link to a DataTransmitter class)
    DataChannel dataChannel(name, publishesPerBatch, publishesIgnoredAfterBatch, zmq_address);

    DataChannelProcessesManager processesManager(channelConfig["num-events-in-circular-buffer"].get<size_t>() + 1, verbose);
    dataChannel.setDataChannelProcessesManager(processesManager);

    // Check if "processors" exist in the channelConfig
    if (channelConfig.contains("processors")) {
        const nlohmann::json& processorsConfig = channelConfig["processors"];

        // Iterate through processors
        for (const auto& processorConfig : processorsConfig) {
            GeneralProcessor* processor;
            if (processorConfig.contains("processor")) {
                std::string processorType = processorConfig["processor"].get<std::string>();
                processor = factory.CreateProcessor(processorType);
            } else {
                printer.PrintWarning("Processor type not found in channel " + channelId + " configuration, using default processor: GeneralProcessor", __LINE__, __FILE__);
                processor = factory.CreateProcessor("GeneralProcessor");
            }
            if (TypeChecker::IsInstanceOf<CommandProcessor>(processor)) {
                // Cast to CommandProcessor
                auto commandProcessor = dynamic_cast<CommandProcessor*>(processor);
                std::string commandString = DEFAULT_COMMAND_STRING;
                if (processorConfig.contains("command")) {
                    commandString = processorConfig["command"].get<std::string>();
                } else {
                    printer.PrintWarning("Command not found in channel " + channelId + " configuration, using default command: None", __LINE__, __FILE__);
                }
                // Create a CommandRunner and set the command
                CommandRunner commandRunner(commandString);

                // Create a CommandProcessor with the CommandRunner
                commandProcessor->setCommandRunner(commandRunner);

                if (processorConfig.contains("period-ms")) {
                    commandProcessor->setPeriod(processorConfig["period-ms"].get<int>());
                } else {
                    printer.PrintWarning("Period not found in channel " + channelId + " configuration, using default period: " + std::to_string(DEFAULT_PERIOD_MS), __LINE__, __FILE__);
                    commandProcessor->setPeriod(DEFAULT_PERIOD_MS);
                }
                dataChannel.addProcessToManager(commandProcessor);

            } else {
                if (processorConfig.contains("period-ms")) {
                    processor->setPeriod(processorConfig["period-ms"].get<int>());
                } else {
                    printer.PrintWarning("Period not found in channel " + channelId + " configuration, using default period: " + std::to_string(DEFAULT_PERIOD_MS), __LINE__, __FILE__);
                    processor->setPeriod(DEFAULT_PERIOD_MS);
                }
                dataChannel.addProcessToManager(processor);
            }
        }
    }
    dataChannel.updateTickTime();
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

int DataChannelManager::getGlobalTickTime() const {
    return globalTickTime;
}

void DataChannelManager::setGlobalTickTime(int tickTime) {
    globalTickTime = tickTime;
}

void DataChannelManager::setGlobalTickTime() {
    // Initialize globalTickTime with the tick time of the first DataChannel
    if (!channels.empty()) {
        globalTickTime = channels.begin()->second.getTickTime();
    } else {
        globalTickTime = 0;
    }

    // Iterate through the channels and find the GCD of their tick times
    for (const auto& channelPair : channels) {
        globalTickTime = std::gcd(globalTickTime, channelPair.second.getTickTime());
    }
}

