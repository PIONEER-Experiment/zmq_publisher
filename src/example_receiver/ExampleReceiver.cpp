#include <zmq.hpp>
#include <iostream>
#include <ProjectPrinter.h>

int main() {
    ProjectPrinter printer;

    zmq::context_t context(1);
    zmq::socket_t subscriberSocket(context, ZMQ_SUB);

    // Connect to the specified address
    const std::string zmqAddress = "tcp://127.0.0.1:5555";
    subscriberSocket.connect(zmqAddress);

    // Subscribe to all messages
    subscriberSocket.set(zmq::sockopt::subscribe, "");

    printer.Print("Connected to address " + zmqAddress + " and subscribed to all messages.");

    while (true) {
        zmq::message_t message;

        // Receive the message
        if (!subscriberSocket.recv(message, zmq::recv_flags::none)) {
            printer.PrintError("Failed to receive a message");
            // Handle the case where recv did not succeed
            continue;
        }

        printer.Print("Received a message of size " + std::to_string(message.size()) + " bytes");

        // Convert the received message to a string
        std::string data(static_cast<char*>(message.data()), message.size());

        // Print the received message
        printer.Print("Message content: " + data);
    }

    return 0;
}
