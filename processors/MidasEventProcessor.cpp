#include "MidasEventProcessor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

MidasEventProcessor::MidasEventProcessor(int verbose)
    : GeneralProcessor(verbose),
      midasReceiver(MidasReceiver::getInstance()),
      lastTimestamp(std::chrono::system_clock::now()) {}

MidasEventProcessor::~MidasEventProcessor() {
    if (initialized) {
        midasReceiver.stop();
    }
}

void MidasEventProcessor::init(
    const std::string& host,
    const std::string& experiment,
    const std::string& buffer,
    const std::string& clientName,
    int eventId,
    bool getAll,
    int bufferSize,
    int yieldTimeoutMs,
    size_t numEventsPerRetrieval
) {
    this->numEventsPerRetrieval = numEventsPerRetrieval;

    midasReceiver.init(
        host,
        experiment,
        buffer,
        clientName,
        eventId,
        getAll,
        bufferSize,
        yieldTimeoutMs
    );
    midasReceiver.start();
    initialized = true;
}

bool MidasEventProcessor::isReadyToProcess() const {
    if (!initialized) {
        return false;
    }
    /*
    // If initialized, it should always be listening for events, unless it was stopped
    // This is not a great solution, it means the "reaction time" of telling the system to stop is
    // The period this class is set to.
    if (!midasReceiver.isListeningForEvents()) {
        midasReceiver.stop();
        return false;
    }
    */
    return true;
}

std::vector<std::string> MidasEventProcessor::getProcessedOutput() {
    std::vector<std::string> output;

    if (!initialized) {
        if (verbose > 0) {
            std::cerr << "[ERROR] MidasEventProcessor called before init()." << std::endl;
        }
        return output;
    }

    auto timedEvents = midasReceiver.getLatestEvents(numEventsPerRetrieval, lastTimestamp);

    if (!timedEvents.empty()) {
        std::ostringstream oss;
        for (const auto& timedEvent : timedEvents) {
            const auto& event = timedEvent.event;
            const auto& timestamp = timedEvent.timestamp;

            oss.str("");
            oss.clear();

            oss << "[EVENT] Event ID: " << event.event_id
                << ", Serial: " << event.serial_number
                << ", Trigger Mask: " << event.trigger_mask
                << ", Timestamp: " << std::chrono::system_clock::to_time_t(timestamp)
                << ", Data Size: " << event.data_size << " bytes";

            if (!event.banks.empty()) {
                oss << ", Banks: " << event.banks.size();
                for (const auto& bank : event.banks) {
                    oss << " [Name: " << bank.name
                        << ", Size: " << bank.data_size << " bytes]";
                }
            }

            oss << ", Data (first 32 bytes): ";
            for (size_t i = 0; i < std::min(event.data.size(), size_t(32)); ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') << (event.data[i] & 0xFF) << " ";
            }
            oss << std::dec;

            output.push_back(oss.str());
        }

        lastTimestamp = timedEvents.back().timestamp;
    } else if (verbose > 0) {
        std::cout << "[INFO] No new events in the buffer.\n";
    }

    return output;
}
