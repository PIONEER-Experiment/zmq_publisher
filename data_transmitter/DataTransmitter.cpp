#include "DataTransmitter.h"
#include "ProjectPrinter.h"

DataTransmitter::DataTransmitter(const std::string& zmqAddress, int verbose)
    : context(1), publisher(context, ZMQ_PUB), zmqAddress(zmqAddress), verbose(verbose) {
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
        return true;
    } catch (const zmq::error_t& e) {
        // Handle any connection errors
        return false;
    }
}

bool DataTransmitter::publish(const std::string& channel, const std::string& data) {
    try {
        // Send the channel (topic)
        zmq::message_t channelMessage(channel.size());
        memcpy(channelMessage.data(), channel.c_str(), channel.size());
        publisher.send(channelMessage, zmq::send_flags::sndmore);

        // Send the actual message content
        zmq::message_t message(data.size());
        memcpy(message.data(), data.c_str(), data.size());
        publisher.send(message, zmq::send_flags::none);

        if (verbose > 1) {
            ProjectPrinter printer;
            printer.Print("Published to channel " + channel + " at address " + zmqAddress + ": " + data);
        }

        return true;
    } catch (const zmq::error_t& e) {
        ProjectPrinter printer;
        printer.PrintError("Failed to send data to address " + zmqAddress, __LINE__, __FILE__);
        return false;
    }
}

void DataTransmitter::setVerbose(int verboseLevel) {
    verbose = verboseLevel;
}
