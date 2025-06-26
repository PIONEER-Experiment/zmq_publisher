#include "MidasEventProcessor.h"
#include <sstream>
#include <iomanip>
#include <iostream>

MidasEventProcessor::MidasEventProcessor(int verbose)
    : GeneralProcessor(verbose),
      midasReceiver(MidasReceiver::getInstance()),
      lastTimestamp(std::chrono::system_clock::now()),
      initialized(false),
      numEventsPerRetrieval(1) {}

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

    MidasReceiverConfig config;
    config.host = host;
    config.experiment = experiment;
    config.bufferName = buffer;
    config.clientName = clientName;
    config.eventID = eventId;
    config.getAllEvents = getAll;
    config.maxBufferSize = bufferSize;
    config.cmYieldTimeout = yieldTimeoutMs;
    // Set transition registrations explicitly (this overrides the defaults)
    config.transitionRegistrations = {
        {TR_START,      100},  // early consumer of START sequence
    };


    midasReceiver.init(config);
    midasReceiver.start();
    initialized = true;
}

bool MidasEventProcessor::isReadyToProcess() const {
    if (!initialized) {
        return false;
    }
    return true;
}

std::string MidasEventProcessor::toHexString(const char* data, size_t size) const {
    std::ostringstream oss;
    for (size_t i = 0; i < size; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << (static_cast<uint8_t>(data[i]) & 0xFF);
    }
    return oss.str();
}

json MidasEventProcessor::decodeBankData(const TMBank& bank, const TMEvent& event) const {
    const char* bankData = event.GetBankData(&bank);
    if (!bankData || bank.data_size == 0) {
        return nullptr;
    }

    size_t dataSize = bank.data_size;
    json dataArray = json::array();

    switch (bank.type) {
        case TID_UINT8: {
            const uint8_t* arr = reinterpret_cast<const uint8_t*>(bankData);
            size_t count = dataSize / sizeof(uint8_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_INT8: {
            const int8_t* arr = reinterpret_cast<const int8_t*>(bankData);
            size_t count = dataSize / sizeof(int8_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_UINT16: {
            const uint16_t* arr = reinterpret_cast<const uint16_t*>(bankData);
            size_t count = dataSize / sizeof(uint16_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_INT16: {
            const int16_t* arr = reinterpret_cast<const int16_t*>(bankData);
            size_t count = dataSize / sizeof(int16_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_UINT32: {
            const uint32_t* arr = reinterpret_cast<const uint32_t*>(bankData);
            size_t count = dataSize / sizeof(uint32_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_INT32: {
            const int32_t* arr = reinterpret_cast<const int32_t*>(bankData);
            size_t count = dataSize / sizeof(int32_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_FLOAT: {
            const float* arr = reinterpret_cast<const float*>(bankData);
            size_t count = dataSize / sizeof(float);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_DOUBLE: {
            const double* arr = reinterpret_cast<const double*>(bankData);
            size_t count = dataSize / sizeof(double);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_INT64: {
            const int64_t* arr = reinterpret_cast<const int64_t*>(bankData);
            size_t count = dataSize / sizeof(int64_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_UINT64: {
            const uint64_t* arr = reinterpret_cast<const uint64_t*>(bankData);
            size_t count = dataSize / sizeof(uint64_t);
            for (size_t i = 0; i < count; ++i) {
                dataArray.push_back(arr[i]);
            }
            break;
        }
        case TID_STRING: {
            dataArray = std::string(bankData, dataSize);
            break;
        }
        default: {
            dataArray = toHexString(bankData, dataSize);
            break;
        }
    }

    return dataArray;
}

std::vector<std::string> MidasEventProcessor::getProcessedOutput() {
    std::vector<std::string> output;

    if (!initialized) {
        return output;
    }

    auto timedEvents = midasReceiver.getLatestEvents(numEventsPerRetrieval, lastTimestamp);

    for (auto& timedEvent : timedEvents) {
        auto& event = timedEvent.event;
        auto& timestamp = timedEvent.timestamp;

        event.FindAllBanks();

        json j;
        j["event_id"] = event.event_id;
        j["serial_number"] = event.serial_number;
        j["trigger_mask"] = event.trigger_mask;
        j["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
        j["data_size"] = event.data_size;
        j["event_header_size"] = event.event_header_size;
        j["bank_header_flags"] = event.bank_header_flags;

        j["banks"] = json::array();

        for (const auto& bank : event.banks) {
            json jbank;
            jbank["name"] = bank.name;
            jbank["type"] = bank.type;
            jbank["data_size"] = bank.data_size;

            jbank["data"] = decodeBankData(bank, event);

            j["banks"].push_back(jbank);
        }

        output.push_back(j.dump());
    }

    if (!timedEvents.empty()) {
        lastTimestamp = timedEvents.back().timestamp;
    }

    return output;
}
