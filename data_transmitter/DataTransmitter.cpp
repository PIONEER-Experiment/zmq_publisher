#include "DataTransmitter.h"
#include "ProjectPrinter.h"

DataTransmitter::DataTransmitter(const std::string& zmqAddress)
    : context(1), publisher(context, ZMQ_PUB), zmqAddress(zmqAddress) {
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

bool DataTransmitter::publish(const std::string& data) {
    try {
        zmq::message_t message(data.size());
        memcpy(message.data(), data.c_str(), data.size());
        publisher.send(message, zmq::send_flags::none);
        ProjectPrinter printer;
        printer.Print("Published to address " + zmqAddress + ": " + data);
        return true;
    } catch (const zmq::error_t& e) {
        // Handle any send errors
        return false;
    }
}
