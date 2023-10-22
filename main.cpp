#include "event_processor/EventProcessor.h"
#include "data_transmitter/DataTransmitter.h"
#include "data_transmitter/DataBuffer.h"
#include "midas_connector/MidasConnector.h"
#include "utilities/ProjectPrinter.h"
#include "json.hpp"
#include <fstream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define CONFIG_FILE "config.json" // Define the configuration file constant
ProjectPrinter printer; //Command line printing tools for the project

// Function to initialize MIDAS and open an event buffer
bool initializeMidas(MidasConnector& midasConnector, const nlohmann::json& config) {
    // Set the MidasConnector properties based on the config
    midasConnector.setEventId(config["eventId"].get<short>());
    midasConnector.setTriggerMask(config["triggerMask"].get<short>());
    midasConnector.setSamplingType(config["samplingType"].get<int>());
    midasConnector.setBufferSize(config["bufferSize"].get<int>());
    midasConnector.setBufferName(config["bufferName"].get<std::string>().c_str());
    midasConnector.setBufferSize(config["bufferSize"].get<int>());

    // Call the ConnectToExperiment method
    if (!midasConnector.ConnectToExperiment()) {
        return false;
    }

    // Call the OpenEventBuffer method
    if (!midasConnector.OpenEventBuffer()) {
        return false;
    }

    // Set the buffer cache size if requested
    midasConnector.SetCacheSize(config["cacheSize"].get<int>());

    // Place a request for a specific event id
    if (!midasConnector.RequestEvent()) {
        return false;
    }

    return true;
}

// Function to get the configuration file path
std::string getConfigFilePath() {
    // Get the directory of the source file
    std::string sourceDirectory = __FILE__;
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
        printer.PrintError(errorMessage, __LINE__, __FILENAME__);
        throw std::runtime_error(errorMessage);
    } else {
        configFile >> config;
        configFile.close();
    }
    return config;
}

int main() {

    // Read configuration from the JSON file
    nlohmann::json config = readConfigFile();

    // Initialize MidasConnector and connect to the MIDAS experiment
    MidasConnector midasConnector(config["clientName"].get<std::string>().c_str());
    if (!initializeMidas(midasConnector, config)) {
        printer.PrintError("Failed to initialize MIDAS.", __LINE__, __FILE__);
        return 1;
    }

    // Read the maximum event size from the JSON configuration
    INT max_event_size = config["maxEventSize"].get<int>();

    // Allocate memory for storing event data dynamically
    void* event_data = malloc(max_event_size);

    // Initialize EventProcessor with detector mapping file and verbosity flag
    EventProcessor eventProcessor(config["detectorMappingFile"].get<std::string>(), config["verbose"].get<bool>());

    // Initialize DataTransmitter with the ZeroMQ address
    DataTransmitter dataPublisher(config["zmqAddress"].get<std::string>());

    // Initialize DataBuffer with a specified buffer size
    DataBuffer<std::string> eventBuffer(config["numEventsInBuffer"].get<size_t>());

    // Connect to the ZeroMQ server
    if (!dataPublisher.bind()) {
        // Handle connection error
        printer.PrintError("Failed to bind to port " + config["zmqAddress"].get<std::string>(), __LINE__, __FILE__);
        return 1;
    } else {
        printer.Print("Connected to the ZeroMQ server.");
    }

    // Event processing loop
    while (true) {

        midasConnector.ReceiveEvent(event_data, max_event_size);

        //Prcoess data once we have it
        eventProcessor.processEvent(event_data, max_event_size);

        // Serialize the event data with EventProcessor and store it in serializedData
        std::string serializedData = eventProcessor.getSerializedData();

        // Add serialized data to the buffer
        eventBuffer.Push(serializedData);
        std::string bufferData = eventBuffer.SerializeBuffer();

        // Send the serialized data to the ZeroMQ server with DataTransmitter
        if (!dataPublisher.publish(bufferData)) {
            printer.PrintError("Failed to send serialized data.", __LINE__, __FILE__);
        }
    }

    // Cleanup and finalize your application
    midasConnector.DisconnectFromExperiment(); // Disconnect from the MIDAS experiment

    return 0;
}
