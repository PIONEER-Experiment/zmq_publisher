/**
 * @file main.cpp
 * @brief Entry point for the program.
 */

// Project Headers needed to run Main
#include "ProjectPrinter.h"
#include "JsonManager.h"
#include "DataTransmitterManager.h"
#include "DataChannelManager.h"
#include "SignalHandler.h"
#include "GeneralProcessorFactory.h"

// Project Headers for processors
#include "GeneralProcessor.h"
#include "CommandProcessor.h"

// Standard Libraries
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <thread>

using json = nlohmann::json;

/**
 * @brief Function to register processor classes.
 *
 * New processors MUST be registered here!
 *
 * @param config Configuration data in JSON format.
 */
void registerProcessors(nlohmann::json config) {
    int verbose = config["general-settings"]["verbose"].get<int>();

    // Get the instance of the GeneralProcessorFactory
    GeneralProcessorFactory& factory = GeneralProcessorFactory::Instance();

    // Register GeneralProcessor with a lambda function creating an instance
    factory.RegisterProcessor("GeneralProcessor", [verbose]() -> GeneralProcessor* { return new GeneralProcessor(verbose); });

    // Register CommandProcessor with a lambda function creating an instance
    factory.RegisterProcessor("CommandProcessor", [verbose]() -> CommandProcessor* { return new CommandProcessor(verbose); });
}

/**
 * @brief The main function of the program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Exit code.
 */
int main(int argc, char* argv[]) {
    // Create an instance of ProjectPrinter
    ProjectPrinter printer;

    // Get cleaned up config
    nlohmann::json config = JsonManager::getInstance().getConfig();
    
    // Get verbosity level from configuration
    int verbose = config["general-settings"]["verbose"].get<int>();

    // Initialize the DataTransmitterManager
    DataTransmitterManager::Instance(config["general-settings"]["verbose"].get<int>());

    // Register processors so we can map strings to processor objects
    registerProcessors(config);

    // Initialize DataChannelManager with configuration and verbosity level
    DataChannelManager dataChannelManager(config["data-channels"], config["general-settings"]["verbose"].get<int>());

    // Set the global tick time
    dataChannelManager.setGlobalTickTime();
    int tickTime = dataChannelManager.getGlobalTickTime();

    // Main loop
    while (!SignalHandler::getInstance().isQuitSignalReceived()) {
        // Publish data
        dataChannelManager.publish();

        // Print message if verbose
        if (verbose > 0) {
            printer.Print("Finished loop, sleeping for " + std::to_string(tickTime) + "ms ...");
        }

        // Sleep for the specified tick time
        std::this_thread::sleep_for(std::chrono::milliseconds(tickTime));
    }

    // Print message and exit
    printer.Print("Received quit signal. Exiting the loop and ending program.");
    return 0;
}
