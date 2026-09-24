#include "zmq_publisher/extensions/midas/midas_processor.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace zmq_publisher::midas {

std::mutex MidasProcessor::instance_mutex_;
bool MidasProcessor::instance_active_ = false;

MidasProcessor::MidasProcessor(const nlohmann::json& config)
    : receiver_(MidasReceiver::getInstance()),
      last_event_timestamp_(std::chrono::system_clock::now()) {
    {
        std::lock_guard lock(instance_mutex_);
        if (instance_active_) {
            throw std::runtime_error("Only one MIDAS processor may be active");
        }
        instance_active_ = true;
    }

    try {
        MidasReceiverConfig receiver_config;
        receiver_config.host = config.value("host", "");
        receiver_config.experiment = config.value("experiment", "");
        receiver_config.bufferName = config.value("buffer", "SYSTEM");
        receiver_config.clientName = config.value("client-name", "ZmqPublisher");
        receiver_config.eventID = config.value("event-id", -1);
        receiver_config.getAllEvents = config.value("get-all", true);
        receiver_config.maxBufferSize = config.value("buffer-size", 1000);
        receiver_config.cmYieldTimeout = config.value("yield-timeout-ms", 300);
        events_per_retrieval_ = config.value("num-events-per-retrieval", 1U);
        if (events_per_retrieval_ == 0) {
            throw std::invalid_argument("num-events-per-retrieval must be greater than zero");
        }
        receiver_config.transitionRegistrations = {{TR_START, 100}};
        receiver_.init(receiver_config);
        receiver_.start();
        initialized_ = true;
    } catch (...) {
        std::lock_guard lock(instance_mutex_);
        instance_active_ = false;
        throw;
    }
}

MidasProcessor::~MidasProcessor() {
    if (initialized_) {
        receiver_.stop();
    }
    std::lock_guard lock(instance_mutex_);
    instance_active_ = false;
}

bool MidasProcessor::is_ready() const {
    return initialized_;
}

std::vector<std::string> MidasProcessor::process() {
    std::vector<std::string> output;
    auto events = receiver_.getLatestEvents(events_per_retrieval_, last_event_timestamp_);
    output.reserve(events.size());

    for (auto& timed_event : events) {
        auto& event = *timed_event->event;
        event.FindAllBanks();

        nlohmann::json serialized;
        serialized["event_id"] = event.event_id;
        serialized["serial_number"] = event.serial_number;
        serialized["trigger_mask"] = event.trigger_mask;
        serialized["timestamp_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            timed_event->timestamp.time_since_epoch()).count();
        serialized["data_size"] = event.data_size;
        serialized["event_header_size"] = event.event_header_size;
        serialized["bank_header_flags"] = event.bank_header_flags;
        serialized["banks"] = nlohmann::json::array();

        for (const auto& bank : event.banks) {
            serialized["banks"].push_back({
                {"name", bank.name},
                {"type", bank.type},
                {"data_size", bank.data_size},
                {"data", decode_bank(bank, event)},
            });
        }
        output.push_back(serialized.dump());
    }

    if (!events.empty()) {
        last_event_timestamp_ = events.back()->timestamp;
    }
    return output;
}

nlohmann::json MidasProcessor::decode_bank(const TMBank& bank, const TMEvent& event) {
    const char* data = event.GetBankData(&bank);
    if (!data || bank.data_size == 0) {
        return nullptr;
    }

    const auto decode_array = [data, &bank](auto type_tag) {
        using Value = decltype(type_tag);
        const auto* values = reinterpret_cast<const Value*>(data);
        const auto count = bank.data_size / sizeof(Value);
        nlohmann::json result = nlohmann::json::array();
        for (std::size_t index = 0; index < count; ++index) {
            result.push_back(values[index]);
        }
        return result;
    };

    switch (bank.type) {
        case TID_UINT8: return decode_array(std::uint8_t{});
        case TID_INT8: return decode_array(std::int8_t{});
        case TID_UINT16: return decode_array(std::uint16_t{});
        case TID_INT16: return decode_array(std::int16_t{});
        case TID_UINT32: return decode_array(std::uint32_t{});
        case TID_INT32: return decode_array(std::int32_t{});
        case TID_FLOAT: return decode_array(float{});
        case TID_DOUBLE: return decode_array(double{});
        case TID_INT64: return decode_array(std::int64_t{});
        case TID_UINT64: return decode_array(std::uint64_t{});
        case TID_STRING: return std::string(data, bank.data_size);
        default: return to_hex(data, bank.data_size);
    }
}

std::string MidasProcessor::to_hex(const char* data, std::size_t size) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < size; ++index) {
        stream << std::hex << std::setw(2) << std::setfill('0')
               << (static_cast<unsigned>(static_cast<unsigned char>(data[index])) & 0xffU);
    }
    return stream.str();
}

}  // namespace zmq_publisher::midas
