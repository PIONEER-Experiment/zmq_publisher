/**
 * @file main.cpp
 * @brief Entry point for the program.
 * 
 *  This file contains the main function and initializes the necessary components
 * for the program to run. It sets up the data channel manager, registers processors,
 * and enters the main loop for data processing.
 * 
 *  @details Midas is a pain to work with, so this additional branch is a "workaround"
 *  to use it. Midas works by handling a global state, which means we need to manage it in main
 *  as well as in the processors. 
 * 
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
#include "MidasEventProcessor.h"

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
    factory.RegisterProcessor("CommandProcessor", [verbose]() -> GeneralProcessor* { return new CommandProcessor(verbose); });

    // Register MidasEventProcessor
    factory.RegisterProcessor("MidasEventProcessor", [verbose]() -> GeneralProcessor* {
        return new MidasEventProcessor(verbose);
    });
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
    // Runs either are true:
    // 1) Quit signal not recieved
    // 2) Midas Reciever is running and listening for events
    while (!SignalHandler::getInstance().isQuitSignalReceived() && 
        (MidasReceiver::getInstance().isListeningForEvents() || !MidasReceiver::getInstance().IsRunning())) {

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
    MidasReceiver::getInstance().stop(); // clean up midas client if it exists
    printer.Print("Received quit signal. Exiting the loop and ending program.");
    return 0;
}
