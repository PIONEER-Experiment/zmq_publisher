#include "zmq_publisher/extensions/midas/midas_processor_config.h"

#include <stdexcept>

namespace zmq_publisher::midas {

MidasProcessorConfig MidasProcessorConfig::from_configuration(
    const ConfigurationView& config) {
    config.validate_keys({
        "host",
        "experiment",
        "buffer",
        "client-name",
        "event-id",
        "get-all",
        "buffer-size",
        "read-cache-size-bytes",
        "yield-timeout-ms",
        "num-events-per-retrieval",
    });

    MidasProcessorConfig result;
    result.host_ = config.optional<std::string>("host", {});
    result.experiment_ = config.optional<std::string>("experiment", {});
    result.buffer_name_ = config.optional("buffer", result.buffer_name_);
    result.client_name_ = config.optional("client-name", result.client_name_);
    result.event_id_ = config.optional("event-id", result.event_id_);
    result.get_all_events_ = config.optional("get-all", result.get_all_events_);
    result.buffer_size_ = config.optional("buffer-size", result.buffer_size_);
    result.read_cache_size_bytes_ =
        config.optional("read-cache-size-bytes", result.read_cache_size_bytes_);
    result.yield_timeout_ms_ =
        config.optional("yield-timeout-ms", result.yield_timeout_ms_);
    result.events_per_retrieval_ =
        config.optional("num-events-per-retrieval", result.events_per_retrieval_);

    if (result.buffer_name_.empty()) {
        throw std::invalid_argument("MIDAS processor buffer cannot be empty");
    }
    if (result.client_name_.empty()) {
        throw std::invalid_argument("MIDAS processor client-name cannot be empty");
    }
    if (result.buffer_size_ <= 0) {
        throw std::invalid_argument("MIDAS processor buffer-size must be greater than zero");
    }
    if (result.yield_timeout_ms_ < 0) {
        throw std::invalid_argument("MIDAS processor yield-timeout-ms cannot be negative");
    }
    if (result.events_per_retrieval_ == 0) {
        throw std::invalid_argument(
            "MIDAS processor num-events-per-retrieval must be greater than zero");
    }
    return result;
}

}  // namespace zmq_publisher::midas
