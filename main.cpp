#include "event_processor/EventProcessor.h"
#include "data_transmitter/DataTransmitter.h"
#include "data_transmitter/DataBuffer.h"
#include "midas_connector/MidasConnector.h"
#include "utilities/ProjectPrinter.h"
#include "utilities/EventLoopManager.h"
#include "json.hpp"
#include "odbxx.h"
#include <fstream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define CONFIG_FILE "config.json" // Define the configuration file constant
ProjectPrinter printer; // Command line printing tools for the project

// Function to initialize MIDAS and open an event buffer
bool initializeMidas(MidasConnector& midasConnector, const nlohmann::json& config) {
    // Set the MidasConnector properties based on the config
    midasConnector.setEventId(config["event-id"].get<short>());
    midasConnector.setTriggerMask(config["trigger-mask"].get<short>());
    midasConnector.setSamplingType(config["sampling-type"].get<int>());
    midasConnector.setBufferSize(config["buffer-size"].get<int>());
    midasConnector.setBufferName(config["buffer-name"].get<std::string>().c_str());
    midasConnector.setBufferSize(config["buffer-size"].get<int>());
    midasConnector.setTimeout(config["timeout-millis"].get<int>());
    //Broken currently
    //midasConnector.SetWatchdogParams(config["call-watchdog"].get<bool>(),static_cast<DWORD>(config["watchdog-timeout-millis"].get<int>()));

    // Call the ConnectToExperiment method
    if (!midasConnector.ConnectToExperiment()) {
        return false;
    }

    // Call the OpenEventBuffer method
    if (!midasConnector.OpenEventBuffer()) {
        return false;
    }

    // Set the buffer cache size if requested
    midasConnector.SetCacheSize(config["cache-size"].get<int>());

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
        try {
            configFile >> config;
            configFile.close();
        } catch (const nlohmann::json::exception& e) {
            std::string errorMessage = "Error while parsing JSON: " + std::string(e.what());
            printer.PrintError(errorMessage, __LINE__, __FILENAME__);
            throw std::runtime_error(errorMessage);
        }
    }
    return config;
}

int main() {
    // Read configuration from the JSON file
    nlohmann::json config = readConfigFile();


    // Initialize MidasConnector and connect to the MIDAS experiment
    MidasConnector midasConnector(config["client-name"].get<std::string>().c_str());
    if (!initializeMidas(midasConnector, config)) {
        printer.PrintError("Failed to initialize MIDAS.", __LINE__, __FILE__);
        return 1;
    }

    //Get the ODB
    midas::odb exp("/");
    std::string odb_json = exp.dump();

    // Read the maximum event size from the JSON configuration
    INT max_event_size = config["max-event-size"].get<int>();

    // Allocate memory for storing event data dynamically
    void* event_data = malloc(max_event_size);

    // Initialize EventProcessor with detector mapping file and verbosity flag
    EventProcessor eventProcessor(config["detector-mapping-file"].get<std::string>(), config["verbose"].get<int>());

    // Initialize DataTransmitter with the ZeroMQ address
    DataTransmitter dataPublisher(config["zmq-address"].get<std::string>(), config["verbose"].get<int>());

    // Initialize DataBuffer with a specified buffer size
    DataBuffer<std::string> eventBuffer(config["num-events-in-buffer"].get<size_t>() + 1);

    //Initialize EventLoopManager with specified events per sleep time and sleep duration
    EventLoopManager eventLoopManager(config["events-before-sleep"].get<int>(),config["sleep-time-millis"].get<int>(),config["timeout-millis"].get<int>(),config["verbose"].get<int>());


    // Connect to the ZeroMQ server
    if (!dataPublisher.bind()) {
        // Handle connection error
        printer.PrintError("Failed to bind to port " + config["zmq-address"].get<std::string>(), __LINE__, __FILENAME__);
        return 1;
    } else {
        printer.Print("Connected to the ZeroMQ server.");
    }

    // Event processing loop
    while (true) {
        int success = midasConnector.ReceiveEvent(event_data,max_event_size);
        if (success == BM_SUCCESS) {
            // Process data once we have it
            eventProcessor.processEvent(event_data, max_event_size);

            // Serialize the event data with EventProcessor and store it in serializedData
            std::string serializedData = eventProcessor.getSerializedData();

            // Add serialized data to the buffer
            eventBuffer.Push(serializedData);
            std::string bufferData = eventBuffer.SerializeBuffer();

            // Send the serialized data to the ZeroMQ server with DataTransmitter
            if (!dataPublisher.publish(config["zmq-data-channel-name"].get<std::string>(), bufferData)) {
                printer.PrintError("Failed to send serialized data to channel: " + config["zmq-data-channel-name"].get<std::string>(), __LINE__, __FILENAME__);
            }
            if (!dataPublisher.publish(config["zmq-odb-channel-name"].get<std::string>(),odb_json)) {
                printer.PrintError("Failed to send serialized data to channel: " + config["zmq-odb-channel-name"].get<std::string>(), __LINE__, __FILENAME__);
            }
        }
        eventLoopManager.ManageLoop(success);
    }

    // Cleanup and finalize your application
    midasConnector.DisconnectFromExperiment(); // Disconnect from the MIDAS experiment

    return 0;
}
