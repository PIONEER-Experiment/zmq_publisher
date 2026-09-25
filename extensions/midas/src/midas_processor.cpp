#include "zmq_publisher/extensions/midas/midas_processor.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace zmq_publisher::midas {

std::mutex MidasProcessor::instance_mutex_;
bool MidasProcessor::instance_active_ = false;

MidasProcessor::MidasProcessor(const MidasProcessorConfig& config)
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
        receiver_config.host = config.host();
        receiver_config.experiment = config.experiment();
        receiver_config.bufferName = config.buffer_name();
        receiver_config.clientName = config.client_name();
        receiver_config.eventID = config.event_id();
        receiver_config.getAllEvents = config.get_all_events();
        receiver_config.maxBufferSize = config.buffer_size();
        receiver_config.cmYieldTimeout = config.yield_timeout_ms();
        events_per_retrieval_ = config.events_per_retrieval();
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

ProcessResult MidasProcessor::process() {
    std::vector<OutputMessage> output;
    auto events = receiver_.getLatestEvents(events_per_retrieval_, last_event_timestamp_);
    output.reserve(events.size());

    for (auto& timed_event : events) {
        auto& event = *timed_event->event;
        ByteBuffer payload(event.data.size());
        std::transform(event.data.begin(), event.data.end(), payload.begin(), [](auto value) {
            return static_cast<std::byte>(static_cast<unsigned char>(value));
        });
        if (!payload.empty()) {
            output.emplace_back(
                std::move(payload),
                "application/x-midas-event",
                "midas-tmevent-v1");
        }
    }

    if (!events.empty()) {
        last_event_timestamp_ = events.back()->timestamp;
    }
    return output.empty()
        ? ProcessResult::idle()
        : ProcessResult::produced(std::move(output));
}

}  // namespace zmq_publisher::midas
