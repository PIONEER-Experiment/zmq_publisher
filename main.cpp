#include "event_processor/EventProcessor.h"
#include "data_transmitter/DataTransmitter.h"
#include "midas_connector/MidasConnector.h"
#include "json.hpp"
#include <fstream>

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

int main() {
    // Read configuration from a JSON file
    nlohmann::json config;
    std::ifstream configFile("config.json");
    configFile >> config;
    configFile.close();

    // Initialize MidasConnector and connect to the MIDAS experiment
    MidasConnector midasConnector(config["clientName"].get<std::string>().c_str());
    if (!initializeMidas(midasConnector, config)) {
        printf("Error: Failed to initialize MIDAS.\n");
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

    // Connect to the ZeroMQ server
    if (!dataPublisher.bind()) {
        // Handle connection error
        printf("Error: Failed to bind to port %s.\n", config["zmqAddress"].get<std::string>().c_str());
        return 1;
    } else {
        printf("Connected to the ZeroMQ server.\n");
    }


    // Event processing loop
    while (true) {

        midasConnector.ReceiveEvent(event_data, max_event_size);

        //Prcoess data once we have it
        eventProcessor.processEvent(event_data, max_event_size);

        // Serialize the event data with EventProcessor and store it in serializedData
        std::string serializedData = eventProcessor.getSerializedData();

        // Send the serialized data to the ZeroMQ server with DataTransmitter
        if (!dataPublisher.publish(serializedData)) {
            // Handle send error
            printf("Error: Failed to send serialized data.\n");
        }
    }

    // Cleanup and finalize your application
    midasConnector.DisconnectFromExperiment(); // Disconnect from the MIDAS experiment

    return 0;
}
