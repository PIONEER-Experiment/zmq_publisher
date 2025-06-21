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
 */

// Project Headers needed to run Main
#include "utilities/JsonManager.h"
#include "data_transmitter/DataTransmitterManager.h"
#include "data_transmitter/DataChannelManager.h"
#include "utilities/SignalHandler.h"
#include "processors/GeneralProcessorFactory.h"
#include "utilities/LoggerConfig.h"

// Project Headers for processors
#include "processors/GeneralProcessor.h"
#include "processors/CommandProcessor.h"
#include "processors/MidasEventProcessor.h"

// Logging
#include <spdlog/spdlog.h>

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

    factory.RegisterProcessor("GeneralProcessor", [verbose]() -> GeneralProcessor* {
        return new GeneralProcessor(verbose);
    });

    factory.RegisterProcessor("CommandProcessor", [verbose]() -> GeneralProcessor* {
        return new CommandProcessor(verbose);
    });

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
    utils::LoggerConfig::ConfigureFromFile();

    // Get cleaned up config
    JsonManager::getInstance();
    nlohmann::json config = JsonManager::getInstance().getConfig();

    // Get verbosity level
    int verbose = config["general-settings"]["verbose"].get<int>();

    // Set global log level
    if (verbose == 0) spdlog::set_level(spdlog::level::warn);
    else if (verbose == 1) spdlog::set_level(spdlog::level::info);
    else spdlog::set_level(spdlog::level::debug);

    spdlog::info("Starting main program with verbosity level {}", verbose);

    // Initialize the DataTransmitterManager
    DataTransmitterManager::Instance(verbose);

    // Register processors
    registerProcessors(config);

    // Initialize DataChannelManager
    DataChannelManager dataChannelManager(config["data-channels"], verbose);

    // Set global tick time
    dataChannelManager.setGlobalTickTime();
    int tickTime = dataChannelManager.getGlobalTickTime();

    // Variables for timing statistics
    size_t loopCount = 0;
    std::chrono::microseconds totalDuration(0);

    // Main loop
    while (!SignalHandler::getInstance().isQuitSignalReceived() && 
           (MidasReceiver::getInstance().isListeningForEvents() || !MidasReceiver::getInstance().IsRunning())) {

        auto start = std::chrono::high_resolution_clock::now();
        dataChannelManager.publish();
        auto end = std::chrono::high_resolution_clock::now();

        totalDuration += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        ++loopCount;

        if (verbose > 0) {
            spdlog::debug("Finished loop, sleeping for {}ms ...", tickTime);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(tickTime));
    }

    // Clean up and exit
    spdlog::info("Received quit signal or MidasReceiver is not running. Stopping MidasReceiver...");
    MidasReceiver::getInstance().stop();

    // Print timing summary
    if (loopCount > 0) {
        double avgMillis = totalDuration.count() / 1000.0 / loopCount;
        spdlog::info("Average publish time over {} iterations: {:.3f} ms", loopCount, avgMillis);
    }


    spdlog::info("Exiting main program.");
    return 0;
}
