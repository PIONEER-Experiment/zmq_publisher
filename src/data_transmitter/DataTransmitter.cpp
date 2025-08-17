#include "data_transmitter/DataTransmitter.h"
#include <spdlog/spdlog.h>

DataTransmitter::DataTransmitter(const std::string& zmqAddress, int verbose)
    : context(1), publisher(context, ZMQ_PUB), zmqAddress(zmqAddress), verbose(verbose), isBoundToSocket(false) {
    // Constructor initializes ZeroMQ socket
}

DataTransmitter::~DataTransmitter() {
    // Destructor cleans up resources
    publisher.close();
    context.close();
}

bool DataTransmitter::bind() {
    try {
        publisher.bind(zmqAddress);
        isBoundToSocket = true;
        return true;
    } catch (const zmq::error_t& e) {
        spdlog::error("Failed to bind to ZMQ address {}: {}", zmqAddress, e.what());
        return false;
    }
}

bool DataTransmitter::isBound() {
    return isBoundToSocket;
}

bool DataTransmitter::publish(DataChannel& dataChannel, const std::string& data) {
    try {
        std::string channel = dataChannel.getName();
        dataChannel.seen();
        if (verbose > 0) {
            std::string channelDetails;
            channelDetails += "Channel Name: " + channel + "\n";
            channelDetails += "Events Before Break: " + std::to_string(dataChannel.getEventsBeforeBreak()) + "\n";
            channelDetails += "Events To Ignore In Break: " + std::to_string(dataChannel.getEventsToIgnoreInBreak()) + "\n";
            channelDetails += "Events Published: " + std::to_string(dataChannel.getEventsPublished()) + "\n";
            channelDetails += "Events Seen: " + std::to_string(dataChannel.getEventsSeen()) + "\n";
            
            if (dataChannel.isOnBreak()) {
                int eventsOnBreak = dataChannel.getEventsToIgnoreInBreak() - dataChannel.getEventsSeenOnBreak();
                channelDetails += channel + " is on a break for " + std::to_string(eventsOnBreak) + " events\n";
            }
            spdlog::debug(channelDetails);
        }

        if (dataChannel.isOnBreak()) {
            return true;
        }

        if (!channel.empty()) {
            zmq::message_t channelMessage(channel.size());
            memcpy(channelMessage.data(), channel.c_str(), channel.size());
            publisher.send(channelMessage, zmq::send_flags::sndmore);
        }

        zmq::message_t message(data.size());
        memcpy(message.data(), data.c_str(), data.size());
        publisher.send(message, zmq::send_flags::none);

        dataChannel.published();

        if (verbose > 2) {
            spdlog::debug("Published to channel {} at address {}: {}", channel, zmqAddress, data);
        } else if (verbose > 1) {
            if (data.length() > 1000) {
                std::string truncatedData = data.substr(0, 1000);
                spdlog::debug("Published to channel {} at address {}: {}... <truncated> ...", channel, zmqAddress, truncatedData);
            } else {
                spdlog::debug("Published to channel {} at address {}: {}", channel, zmqAddress, data);
            }
        } else if (verbose > 0) {
            spdlog::debug("Published to channel {} at address {}", channel, zmqAddress);
        }

        return true;
    } catch (const zmq::error_t& e) {
        spdlog::error("Failed to send data to address {}: {}", zmqAddress, e.what());
        return false;
    }
}

void DataTransmitter::setVerbose(int verboseLevel) {
    verbose = verboseLevel;
}
