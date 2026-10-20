#include "zmq_publisher/extensions/gaudi/gaudi_processor_config.h"

#include <stdexcept>

namespace zmq_publisher::gaudi {

GaudiProcessorConfig GaudiProcessorConfig::from_configuration(
    const ConfigurationView& config) {
    config.validate_keys({"midas", "processing", "publication", "sidecar"});
    const auto midas = config.object("midas");
    const auto processing = config.object("processing");
    const auto publication = config.object("publication");
    const auto sidecar = config.object("sidecar");
    midas.validate_keys({
        "host", "experiment", "buffer", "client-name", "event-id", "get-all",
        "buffer-size", "read-cache-size-bytes", "yield-timeout-ms"});
    processing.validate_keys({"events-per-request", "maximum-batch-bytes"});
    publication.validate_keys({"every-events", "every-ms"});
    sidecar.validate_keys({
        "socket-path", "connect-timeout-ms", "request-timeout-ms",
        "launcher", "job-options", "startup-timeout-ms"});

    GaudiProcessorConfig result;
    result.host_ = midas.optional<std::string>("host", {});
    result.experiment_ = midas.optional<std::string>("experiment", {});
    result.buffer_name_ = midas.optional("buffer", result.buffer_name_);
    result.client_name_ = midas.optional("client-name", result.client_name_);
    result.event_id_ = midas.optional("event-id", result.event_id_);
    result.get_all_events_ = midas.optional("get-all", result.get_all_events_);
    result.buffer_size_ = midas.optional("buffer-size", result.buffer_size_);
    result.read_cache_size_bytes_ =
        midas.optional("read-cache-size-bytes", result.read_cache_size_bytes_);
    result.yield_timeout_ms_ = midas.optional("yield-timeout-ms", result.yield_timeout_ms_);
    result.events_per_request_ =
        processing.optional("events-per-request", result.events_per_request_);
    result.maximum_batch_bytes_ =
        processing.optional("maximum-batch-bytes", result.maximum_batch_bytes_);
    result.publish_every_events_ =
        publication.optional("every-events", result.publish_every_events_);
    result.publish_every_ms_ =
        publication.optional("every-ms", result.publish_every_ms_);
    result.socket_path_ = sidecar.required<std::string>("socket-path");
    result.connect_timeout_ms_ =
        sidecar.optional("connect-timeout-ms", result.connect_timeout_ms_);
    result.request_timeout_ms_ =
        sidecar.optional("request-timeout-ms", result.request_timeout_ms_);
    result.launcher_ = sidecar.optional<std::string>("launcher", {});
    result.job_options_ = sidecar.optional<std::string>("job-options", {});
    result.startup_timeout_ms_ =
        sidecar.optional("startup-timeout-ms", result.startup_timeout_ms_);

    if (result.buffer_name_.empty() || result.client_name_.empty() ||
        result.socket_path_.empty()) {
        throw std::invalid_argument("Gaudi processor names and socket path cannot be empty");
    }
    if (result.buffer_size_ == 0 || result.events_per_request_ == 0 ||
        result.maximum_batch_bytes_ == 0) {
        throw std::invalid_argument("Gaudi processor buffer and batch sizes must be positive");
    }
    if (result.publish_every_ms_ < 0 || result.yield_timeout_ms_ < 0 ||
        result.connect_timeout_ms_ <= 0 ||
        result.request_timeout_ms_ <= 0 || result.startup_timeout_ms_ <= 0) {
        throw std::invalid_argument("Gaudi processor timeouts must be positive");
    }
    if (result.launcher_.empty() != result.job_options_.empty()) {
        throw std::invalid_argument(
            "Gaudi sidecar launcher and job-options must be configured together");
    }
    return result;
}

}  // namespace zmq_publisher::gaudi
