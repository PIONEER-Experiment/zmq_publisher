#pragma once

#include <cstddef>
#include "zmq_publisher/config/configuration_view.h"

#include <string>

namespace zmq_publisher::midas {

class MidasProcessorConfig {
public:
    static MidasProcessorConfig from_configuration(const ConfigurationView& config);

    const std::string& host() const noexcept { return host_; }
    const std::string& experiment() const noexcept { return experiment_; }
    const std::string& buffer_name() const noexcept { return buffer_name_; }
    const std::string& client_name() const noexcept { return client_name_; }
    int event_id() const noexcept { return event_id_; }
    bool get_all_events() const noexcept { return get_all_events_; }
    int buffer_size() const noexcept { return buffer_size_; }
    std::size_t read_cache_size_bytes() const noexcept { return read_cache_size_bytes_; }
    int yield_timeout_ms() const noexcept { return yield_timeout_ms_; }
    std::size_t events_per_retrieval() const noexcept { return events_per_retrieval_; }

private:
    std::string host_;
    std::string experiment_;
    std::string buffer_name_ = "SYSTEM";
    std::string client_name_ = "ZmqPublisher";
    int event_id_ = -1;
    bool get_all_events_ = true;
    int buffer_size_ = 1000;
    std::size_t read_cache_size_bytes_ = 100000;
    int yield_timeout_ms_ = 300;
    std::size_t events_per_retrieval_ = 1;
};

}  // namespace zmq_publisher::midas
