#pragma once

#include "zmq_publisher/config/configuration_view.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace zmq_publisher::gaudi {

class GaudiProcessorConfig {
public:
    static GaudiProcessorConfig from_configuration(const ConfigurationView& config);

    const std::string& host() const noexcept { return host_; }
    const std::string& experiment() const noexcept { return experiment_; }
    const std::string& buffer_name() const noexcept { return buffer_name_; }
    const std::string& client_name() const noexcept { return client_name_; }
    int event_id() const noexcept { return event_id_; }
    bool get_all_events() const noexcept { return get_all_events_; }
    std::size_t buffer_size() const noexcept { return buffer_size_; }
    std::size_t read_cache_size_bytes() const noexcept { return read_cache_size_bytes_; }
    int yield_timeout_ms() const noexcept { return yield_timeout_ms_; }
    std::size_t events_per_request() const noexcept { return events_per_request_; }
    std::size_t maximum_batch_bytes() const noexcept { return maximum_batch_bytes_; }
    std::uint64_t publish_every_events() const noexcept { return publish_every_events_; }
    int publish_every_ms() const noexcept { return publish_every_ms_; }
    const std::string& socket_path() const noexcept { return socket_path_; }
    int connect_timeout_ms() const noexcept { return connect_timeout_ms_; }
    int request_timeout_ms() const noexcept { return request_timeout_ms_; }
    const std::string& launcher() const noexcept { return launcher_; }
    const std::string& job_options() const noexcept { return job_options_; }
    int startup_timeout_ms() const noexcept { return startup_timeout_ms_; }

private:
    std::string host_;
    std::string experiment_;
    std::string buffer_name_ = "SYSTEM";
    std::string client_name_ = "GaudiProcessor";
    int event_id_ = -1;
    bool get_all_events_ = true;
    std::size_t buffer_size_ = 1000;
    std::size_t read_cache_size_bytes_ = 100000;
    int yield_timeout_ms_ = 10;
    std::size_t events_per_request_ = 32;
    std::size_t maximum_batch_bytes_ = 16U * 1024U * 1024U;
    std::uint64_t publish_every_events_ = 10000;
    int publish_every_ms_ = 1000;
    std::string socket_path_;
    int connect_timeout_ms_ = 5000;
    int request_timeout_ms_ = 30000;
    std::string launcher_;
    std::string job_options_;
    int startup_timeout_ms_ = 60000;
};

}  // namespace zmq_publisher::gaudi
