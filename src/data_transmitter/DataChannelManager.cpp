#include "DataChannelManager.h"
#include "DataChannelProcessesManager.h"
#include "GeneralProcessorFactory.h"
#include "GeneralProcessor.h"
#include "CommandProcessor.h"
#include "MidasEventProcessor.h"
#include "CommandRunner.h"
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
const bool DEFAULT_ENABLED_VALUE                 = true;

DataChannelManager::DataChannelManager(const nlohmann::json& channelConfig, int verbose) 
    : verbose(verbose) {
    for (auto it = channelConfig.begin(); it != channelConfig.end(); ++it) {
        const std::string& channelId = it.key();
        const nlohmann::json& channelData = it.value();
        ProjectPrinter printer;
        printer.Print("Processing " + channelId + "...");

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
    GeneralProcessorFactory& factory = GeneralProcessorFactory::Instance();

    // Read basic channel parameters with getOrDefault
    bool enabled = getOrDefault(channelConfig, "enabled", DEFAULT_ENABLED_VALUE, channelId, "channel config");
    if (!enabled) return;

    std::string name = getOrDefault(channelConfig, "name", std::string(DEFAULT_NAME), channelId, "channel config");
    int publishesPerBatch = getOrDefault(channelConfig, "publishes-per-batch", DEFAULT_PUBLISHES_PER_BATCH, channelId, "channel config");
    int publishesIgnoredAfterBatch = getOrDefault(channelConfig, "publishes-ignored-after-batch", DEFAULT_PUBLISHES_IGNORED_AFTER_BATCH, channelId, "channel config");
    std::string zmq_address = getOrDefault(channelConfig, "zmq-address", std::string(DEFAULT_ZMQ_ADDRESS), channelId, "channel config");
    int eventsInCircularBuffer = getOrDefault(channelConfig, "num-events-in-circular-buffer", DEFAULT_EVENTS_IN_CIRCULAR_BUFFER, channelId, "channel config");

    // Initialize DataChannel and ProcessesManager
    DataChannel dataChannel(name, publishesPerBatch, publishesIgnoredAfterBatch, zmq_address);
    DataChannelProcessesManager processesManager(eventsInCircularBuffer + 1, verbose);
    dataChannel.setDataChannelProcessesManager(processesManager);

    // If processors exist, configure them
    if (channelConfig.contains("processors")) {
        for (const auto& processorConfig : channelConfig["processors"]) {
            GeneralProcessor* processor = nullptr;

            std::string processorType = getOrDefault(processorConfig, "processor", std::string("GeneralProcessor"), channelId, "processor config");

            // Create processor instance from factory
            processor = factory.CreateProcessor(processorType);

            if (!processor) {
                printer.PrintWarning("Failed to create processor '" + processorType + "' in channel " + channelId, __LINE__, __FILE__);
                continue;
            }

            // If MidasEventProcessor, initialize with its special params
            if (TypeChecker::IsInstanceOf<MidasEventProcessor>(processor)) {
                auto* midasProcessor = dynamic_cast<MidasEventProcessor*>(processor);
                if (!midasProcessor) {
                    printer.PrintWarning("Failed to cast to MidasEventProcessor in channel " + channelId, __LINE__, __FILE__);
                    delete processor;
                    continue;
                }

                std::string host = getOrDefault(processorConfig, "host", std::string(""), channelId, "processor config");
                std::string experiment = getOrDefault(processorConfig, "experiment", std::string(""), channelId, "processor config");
                std::string buffer = getOrDefault(processorConfig, "buffer", std::string("SYSTEM"), channelId, "processor config");
                std::string clientName = getOrDefault(processorConfig, "client-name", std::string("MidasEventProcessor"), channelId, "processor config");
                int eventId = getOrDefault(processorConfig, "event-id", EVENTID_ALL, channelId, "processor config");
                bool getAll = getOrDefault(processorConfig, "get-all", true, channelId, "processor config");
                int bufferSize = getOrDefault(processorConfig, "buffer-size", 1000, channelId, "processor config");
                int yieldTimeoutMs = getOrDefault(processorConfig, "yield-timeout-ms", 300, channelId, "processor config");
                size_t numEvents = getOrDefault(processorConfig, "num-events-per-retrieval", static_cast<size_t>(1), channelId, "processor config");

                midasProcessor->init(host, experiment, buffer, clientName, eventId, getAll, bufferSize, yieldTimeoutMs, numEvents);

                int periodMs = getOrDefault(processorConfig, "period-ms", DEFAULT_PERIOD_MS, channelId, "processor config");
                midasProcessor->setPeriod(periodMs);

                dataChannel.addProcessToManager(midasProcessor);
            }
            // Handle CommandProcessor specially
            else if (TypeChecker::IsInstanceOf<CommandProcessor>(processor)) {
                auto* commandProcessor = dynamic_cast<CommandProcessor*>(processor);
                if (!commandProcessor) {
                    printer.PrintWarning("Failed to cast to CommandProcessor in channel " + channelId, __LINE__, __FILE__);
                    delete processor;
                    continue;
                }

                std::string commandString = getOrDefault(processorConfig, "command", std::string(DEFAULT_COMMAND_STRING), channelId, "processor config");

                CommandRunner commandRunner(commandString);
                commandProcessor->setCommandRunner(commandRunner);

                int periodMs = getOrDefault(processorConfig, "period-ms", DEFAULT_PERIOD_MS, channelId, "processor config");
                commandProcessor->setPeriod(periodMs);

                dataChannel.addProcessToManager(commandProcessor);
            }
            else {
                // Default for GeneralProcessor and others
                int periodMs = getOrDefault(processorConfig, "period-ms", DEFAULT_PERIOD_MS, channelId, "processor config");
                processor->setPeriod(periodMs);
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

