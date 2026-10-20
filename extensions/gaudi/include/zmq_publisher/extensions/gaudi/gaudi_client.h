#pragma once

#include "zmq_publisher/processor/immutable_payload.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zmq_publisher::gaudi {

struct GaudiProduct {
    ByteBuffer payload;
    std::string content_type;
    std::string schema;
};

struct GaudiResponse {
    struct Timing {
        std::int64_t request_send_nanoseconds = 0;
        std::int64_t response_wait_nanoseconds = 0;
        std::int64_t response_receive_nanoseconds = 0;
        std::int64_t round_trip_nanoseconds = 0;
    };

    std::vector<GaudiProduct> products;
    std::string metadata;
    Timing timing;
};

struct GaudiEventRequest {
    const std::byte* data = nullptr;
    std::size_t size = 0;
    bool publish = false;
};

class GaudiClient {
public:
    GaudiClient(
        std::string socket_path,
        std::chrono::milliseconds connect_timeout,
        std::chrono::milliseconds request_timeout);
    ~GaudiClient();

    GaudiClient(const GaudiClient&) = delete;
    GaudiClient& operator=(const GaudiClient&) = delete;

    GaudiResponse process(const std::byte* event, std::size_t size);
    std::vector<GaudiResponse> process_batch(
        const std::vector<GaudiEventRequest>& events);

private:
    void connect();
    void close() noexcept;

    std::string socket_path_;
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds request_timeout_;
    int socket_ = -1;
    std::uint64_t next_request_id_ = 1;
};

}  // namespace zmq_publisher::gaudi
