//Project Specific headers
#include "EventProcessor.h"
#include "MidasEvent.h"
#include "MidasBank.h"
#include "MdumpPackage.h"
#include "DataTransmitter.h"
#include "DataBuffer.h"
#include "DataChannel.h"
#include "MidasConnector.h"
#include "ODBGrabber.h"
#include "ProjectPrinter.h"
#include "EventLoopManager.h"
#include "CommandRunner.h"
#include "MdumpCommandRunner.h"
#include "SignalHandler.h"
#include "JsonManager.h"
#include "GeneralProcessorFactory.h"
#include "GeneralProcessor.h"
#include "CommandProcessor.h"
#include "MdumpProcessor.h"
#include "ODBProcessor.h"
#include "HistogramProcessor.h"
#include "DataTransmitterManager.h"
#include "DataChannelManager.h"

//Special "External" Headers
#include "midas.h"
#include "midasio.h"
#include "unpackers/BasicEventUnpacker.hh"
#include "unpackers/EventUnpacker.hh"
#include "serializer/Serializer.hh"
#include "dataProducts/Waveform.hh"
#include "odbxx.h"

//Standard Libraries
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

using json = nlohmann::json;

// Function to register processor classes
// New processors MUST be registered here!
void registerProcessors(nlohmann::json config) {
    int verbose = config["general-settings"]["verbose"].get<int>();
    std::string detectorMappingFile = config["data-channels"]["mdump-channel"]["processors"][0]["detector-mapping-file"].get<std::string>();
    GeneralProcessorFactory& factory = GeneralProcessorFactory::Instance();

    factory.RegisterProcessor("GeneralProcessor", [verbose]() -> GeneralProcessor* { return new GeneralProcessor(verbose); });
    factory.RegisterProcessor("CommandProcessor", [verbose]() -> CommandProcessor* { return new CommandProcessor(verbose); });
    factory.RegisterProcessor("MdumpProcessor", [verbose, detectorMappingFile]() -> MdumpProcessor* {
        return new MdumpProcessor(detectorMappingFile, verbose);
    });
    factory.RegisterProcessor("ODBProcessor", [verbose]() -> ODBProcessor* { return new ODBProcessor(verbose); });
    factory.RegisterProcessor("HistogramProcessor", [verbose]() -> HistogramProcessor* { return new HistogramProcessor(verbose); });
}

int generalMode() {
    ProjectPrinter printer;
    nlohmann::json config = JsonManager::getInstance().getConfig(); //Get cleaned up config
    int verbose = config["general-settings"]["verbose"].get<int>();
    DataTransmitterManager::Instance(config["general-settings"]["verbose"].get<int>()); //Initialize the DataTransmitterManager
    registerProcessors(config); //Register processors so we can map strings to processor objects
    DataChannelManager dataChannelManager(config["data-channels"],config["general-settings"]["verbose"].get<int>());
    dataChannelManager.setGlobalTickTime();
    int tickTime = dataChannelManager.getGlobalTickTime();
    while (!SignalHandler::getInstance().isQuitSignalReceived()) {
        dataChannelManager.publish();
        if (verbose > 0) {
            printer.Print("Finished loop, sleeping for " + std::to_string(tickTime) + "ms ...");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(tickTime));     
    }
    printer.Print("Received quit signal. Exiting the loop and ending program.");
    return 0;
}

int main(int argc, char* argv[]) {
    return generalMode();
}