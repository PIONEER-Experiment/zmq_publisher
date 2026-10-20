#include "zmq_publisher/extensions/gaudi/gaudi_processor.h"

#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

namespace zmq_publisher::gaudi {

std::mutex GaudiProcessor::instance_mutex_;
bool GaudiProcessor::instance_active_ = false;

GaudiProcessor::GaudiProcessor(const GaudiProcessorConfig& config)
    : receiver_(MidasReceiver::getInstance()),
      sidecar_(
          config.launcher(),
          config.job_options(),
          config.socket_path(),
          std::chrono::milliseconds(config.startup_timeout_ms())),
      client_(
          config.socket_path(),
          std::chrono::milliseconds(config.connect_timeout_ms()),
          std::chrono::milliseconds(config.request_timeout_ms())),
      events_per_request_(config.events_per_request()),
      maximum_batch_bytes_(config.maximum_batch_bytes()),
      publish_every_events_(config.publish_every_events()),
      publish_every_(config.publish_every_ms()) {
    {
        std::lock_guard lock(instance_mutex_);
        if (instance_active_) {
            throw std::runtime_error("Only one MIDAS-to-Gaudi processor may be active");
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

GaudiProcessor::~GaudiProcessor() {
    if (initialized_) {
        receiver_.setEventAvailableCallback({});
        receiver_.stop();
    }
    std::lock_guard lock(instance_mutex_);
    instance_active_ = false;
}

bool GaudiProcessor::is_ready() const {
    return initialized_;
}

bool GaudiProcessor::has_pending_work() const {
    return receiver_.pendingEventCount() != 0;
}

bool GaudiProcessor::set_wakeup_handler(WakeupHandler handler) {
    receiver_.setEventAvailableCallback(std::move(handler));
    return true;
}

ProcessResult GaudiProcessor::process() {
    auto events = receiver_.takeEvents(events_per_request_);
    if (events.empty()) return ProcessResult::idle();

    std::optional<std::size_t> last_data_event;
    auto event_publication_counter = events_since_publication_;
    std::vector<bool> publish_event(events.size(), false);
    for (std::size_t index = 0; index < events.size(); ++index) {
        const auto event_id = events[index]->event.event_id;
        if (event_id < 0x8000) {
            last_data_event = index;
            ++event_publication_counter;
            if (publish_every_events_ != 0 &&
                event_publication_counter >= publish_every_events_) {
                publish_event[index] = true;
                event_publication_counter = 0;
            }
        }
    }
    const auto now = std::chrono::steady_clock::now();
    const bool time_publication_due = publish_every_.count() != 0 &&
        now - last_publication_ >= publish_every_;
    if (time_publication_due && last_data_event.has_value()) {
        publish_event[*last_data_event] = true;
        event_publication_counter = 0;
    }

    std::vector<GaudiResponse> responses;
    responses.reserve(events.size());
    for (std::size_t begin = 0; begin < events.size();) {
        std::size_t end = begin;
        std::size_t batch_bytes = 0;
        std::vector<GaudiEventRequest> requests;
        while (end < events.size()) {
            const auto& bytes = events[end]->event.data;
            if (end != begin && batch_bytes + bytes.size() > maximum_batch_bytes_) break;
            requests.push_back({
                reinterpret_cast<const std::byte*>(bytes.data()),
                bytes.size(),
                publish_event[end]});
            batch_bytes += bytes.size();
            ++end;
        }
        auto batch_responses = client_.process_batch(requests);
        for (auto& response : batch_responses) {
            responses.push_back(std::move(response));
        }
        begin = end;
    }

    events_since_publication_ = event_publication_counter;
    if (std::find(publish_event.begin(), publish_event.end(), true) != publish_event.end()) {
        last_publication_ = now;
    }

    std::vector<OutputMessage> output;
    output.reserve(2);
    for (std::size_t index = 0; index < events.size(); ++index) {
        const auto& timed_event = events[index];
        const auto& bytes = timed_event->event.data;
        auto& response = responses[index];
        const auto metadata = nlohmann::json::parse(response.metadata);
        for (auto& product : response.products) {
            OutputMessage message(
                std::move(product.payload),
                std::move(product.content_type),
                std::move(product.schema));
            message.add_timing_marker("midas-receiver", timed_event->steadyTimestamp);
            message.add_timing_marker(
                "midas-processor-dequeued", timed_event->dequeuedSteadyTimestamp);
            message.add_timing_marker("gaudi-analysis-completed", std::chrono::steady_clock::now());
            message.set_numeric_attribute("midas.event-id", timed_event->event.event_id);
            message.set_numeric_attribute("midas.serial-number", timed_event->event.serial_number);
            message.set_numeric_attribute("midas.event-bytes", bytes.size());
            message.set_numeric_attribute(
                "gaudi.client.request-send-ns", response.timing.request_send_nanoseconds);
            message.set_numeric_attribute(
                "gaudi.client.response-wait-ns", response.timing.response_wait_nanoseconds);
            message.set_numeric_attribute(
                "gaudi.client.response-receive-ns", response.timing.response_receive_nanoseconds);
            message.set_numeric_attribute(
                "gaudi.client.round-trip-ns", response.timing.round_trip_nanoseconds);
            const auto timing = metadata.value("timing-ns", nlohmann::json::object());
            for (const auto& [source, destination] : {
                     std::pair{"socket-receive", "gaudi.sidecar.socket-receive-ns"},
                     std::pair{"midas-parse", "gaudi.sidecar.midas-parse-ns"},
                     std::pair{"pipeline", "gaudi.sidecar.pipeline-ns"},
                     std::pair{"serialization", "gaudi.sidecar.serialization-ns"}}) {
                if (timing.contains(source)) {
                    message.set_numeric_attribute(destination, timing.at(source).get<std::int64_t>());
                }
            }
            const auto algorithm_timings =
                metadata.value("algorithm-timing-ns", nlohmann::json::object());
            for (const auto& [algorithm, value] : algorithm_timings.items()) {
                if (value.is_number_integer()) {
                    message.set_numeric_attribute(
                        "gaudi.algorithm." + algorithm + "-ns",
                        value.get<std::int64_t>());
                }
            }
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
            "midas-receiver.batch-queue-depth", events.front()->queueDepthAtDequeue);
        last.set_numeric_attribute(
            "midas-receiver.queue-high-watermark", statistics.queueHighWatermark);
        last.set_numeric_attribute(
            "midas-receiver.total-queue-latency-ns",
            statistics.totalQueueLatencyNanoseconds);
    }
    return output.empty()
        ? ProcessResult::processed(events.size())
        : ProcessResult::produced(std::move(output), events.size());
}

}  // namespace zmq_publisher::gaudi
