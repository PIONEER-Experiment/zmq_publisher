#ifndef MIDAS_EVENT_PROCESSOR_H
#define MIDAS_EVENT_PROCESSOR_H

#include "GeneralProcessor.h"
#include "MidasReceiver.h"
#include <chrono>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief A processor that retrieves and processes MIDAS events.
 *
 * This class uses the singleton MidasReceiver to retrieve events and convert them
 * into a vector of strings for further handling.
 */
class MidasEventProcessor : public GeneralProcessor {
public:
    explicit MidasEventProcessor(int verbose = 0);
    ~MidasEventProcessor() override;

    /**
     * @brief Initializes the MIDAS receiver with specified parameters.
     */
    void init(
        const std::string& host = "",
        const std::string& experiment = "",
        const std::string& buffer = "SYSTEM",
        const std::string& clientName = "MidasEventProcessor",
        int eventId = EVENTID_ALL,
        bool getAll = true,
        int bufferSize = 1000,
        int yieldTimeoutMs = 300,
        size_t numEventsPerRetrieval = 10
    );

    std::vector<std::string> getProcessedOutput() override;
    bool isReadyToProcess() const override;

private:
    MidasReceiver& midasReceiver;
    std::chrono::system_clock::time_point lastTimestamp;
    size_t numEventsPerRetrieval = 10;
    bool initialized = false;

    // Private helper methods (formerly static functions)
    std::string toHexString(const char* data, size_t size) const;
    json decodeBankData(const TMBank& bank, const TMEvent& event) const;
};

#endif // MIDAS_EVENT_PROCESSOR_H
