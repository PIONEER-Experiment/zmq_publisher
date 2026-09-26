#include "zmq_publisher/extensions/midas/midas_processor.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace zmq_publisher::midas {

std::mutex MidasProcessor::instance_mutex_;
bool MidasProcessor::instance_active_ = false;

MidasProcessor::MidasProcessor(const MidasProcessorConfig& config)
    : receiver_(MidasReceiver::getInstance()) {
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
        receiver_config.readCacheSizeBytes = config.read_cache_size_bytes();
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
        receiver_.setEventAvailableCallback({});
        receiver_.stop();
    }
    std::lock_guard lock(instance_mutex_);
    instance_active_ = false;
}

bool MidasProcessor::is_ready() const {
    return initialized_;
}

bool MidasProcessor::has_pending_work() const {
    return receiver_.pendingEventCount() != 0;
}

bool MidasProcessor::set_wakeup_handler(WakeupHandler handler) {
    receiver_.setEventAvailableCallback(std::move(handler));
    return true;
}

ProcessResult MidasProcessor::process() {
    std::vector<OutputMessage> output;
    auto events = receiver_.takeEvents(events_per_retrieval_);
    output.reserve(events.size());

    for (auto& timed_event : events) {
        // TMEvent copied the transient MIDAS callback buffer into this vector. Alias that
        // owned storage through the transport so no second event-sized copy is needed.
        auto event_storage = std::shared_ptr<const std::vector<char>>(
            timed_event,
            &timed_event->event.data);
        auto payload = ImmutablePayload::from_shared_vector(std::move(event_storage));
        if (!payload.empty()) {
            OutputMessage message(
                std::move(payload),
                "application/x-midas-event",
                "midas-tmevent-v1");
            message.add_timing_marker("midas-receiver", timed_event->steadyTimestamp);
            message.add_timing_marker(
                "midas-processor-dequeued",
                timed_event->dequeuedSteadyTimestamp);
            output.push_back(std::move(message));
        }
    }
    if (!output.empty()) {
        const auto statistics = receiver_.statistics();
        auto& last = output.back();
        last.set_numeric_attribute("midas-receiver.events-received", statistics.eventsReceived);
        last.set_numeric_attribute("midas-receiver.events-consumed", statistics.eventsConsumed);
        last.set_numeric_attribute("midas-receiver.events-dropped", statistics.eventsDropped);
        last.set_numeric_attribute("midas-receiver.queue-depth", statistics.queueDepth);
        last.set_numeric_attribute(
            "midas-receiver.batch-queue-depth",
            events.front()->queueDepthAtDequeue);
        last.set_numeric_attribute(
            "midas-receiver.queue-high-watermark",
            statistics.queueHighWatermark);
        last.set_numeric_attribute(
            "midas-receiver.total-queue-latency-ns",
            statistics.totalQueueLatencyNanoseconds);
    }
    return output.empty()
        ? ProcessResult::idle()
        : ProcessResult::produced(std::move(output));
}

}  // namespace zmq_publisher::midas
