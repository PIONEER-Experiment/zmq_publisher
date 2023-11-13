#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <ProjectPrinter.h>

using json = nlohmann::json;
#define CONFIG_FILE "config.json" // Define the configuration file constant
ProjectPrinter printer;

std::string getConfigFilePath() {
    // Get the directory of the source file
    std::string sourceDirectory = __FILE__;
    sourceDirectory = sourceDirectory.substr(0, sourceDirectory.find_last_of('/'));

    // Navigate one directory back
    sourceDirectory = sourceDirectory.substr(0, sourceDirectory.find_last_of('/'));

    // Build the full path to the configuration file
    return sourceDirectory + "/" + CONFIG_FILE;
}
// Function to read the configuration from the file
nlohmann::json readConfigFile() {
    std::string configFilePath = getConfigFilePath();
    nlohmann::json config;
    std::ifstream configFile(configFilePath);
    if (!configFile) {
        std::string errorMessage = "Failed to open configuration file: " + configFilePath;
        printer.PrintError(errorMessage, __LINE__, __FILE__);
        throw std::runtime_error(errorMessage);
    } else {
        try {
            configFile >> config;
            configFile.close();
        } catch (const nlohmann::json::exception& e) {
            std::string errorMessage = "Error while parsing JSON: " + std::string(e.what());
            printer.PrintError(errorMessage, __LINE__, __FILE__);
            throw std::runtime_error(errorMessage);
        }
    }
    return config;
}

int main() {
    json config = readConfigFile();

    zmq::context_t context(1);
    zmq::socket_t subscriberSocket(context, ZMQ_SUB);

    std::string zmqAddress = config["data-channels"]["mdump-channel"]["zmq-address"].get<std::string>();
    subscriberSocket.connect(zmqAddress);
    subscriberSocket.set(zmq::sockopt::subscribe, "");

    printer.Print("Connected to address " + zmqAddress + " and subscribed to all messages.");

    while (true) {
        zmq::message_t message;
        
        zmq::recv_result_t result = subscriberSocket.recv(message, zmq::recv_flags::none);

        if (!result) {
            printer.PrintError("Failed to receive a message");
            // Handle the case where recv did not succeed
        } else {
            printer.Print("Received a message of size " + std::to_string(message.size()) + " bytes");
            std::string data(static_cast<char*>(message.data()), message.size());
            printer.Print("Received: " + data);
        }
    }

    return 0;
}