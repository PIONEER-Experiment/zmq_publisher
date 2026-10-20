#include "zmq_publisher/extensions/gaudi/gaudi_client.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <nlohmann/json.hpp>

namespace zmq_publisher::gaudi {
namespace {

// Every stream message starts with a four-character type tag followed by a
// four-digit protocol version. This identifies framing/version mismatches
// before either process interprets sizes or payload bytes.
template <unsigned Version>
constexpr std::array<char, 8> make_protocol_identifier(
    std::array<char, 4> tag) noexcept {
    static_assert(Version <= 9999, "protocol version must fit four digits");
    return {
        tag[0], tag[1], tag[2], tag[3],
        static_cast<char>('0' + (Version / 1000) % 10),
        static_cast<char>('0' + (Version / 100) % 10),
        static_cast<char>('0' + (Version / 10) % 10),
        static_cast<char>('0' + Version % 10),
    };
}

constexpr auto kRequestMagic =
    make_protocol_identifier<2>({'P', 'I', 'G', 'A'});
constexpr auto kResponseMagic =
    make_protocol_identifier<2>({'P', 'I', 'G', 'O'});
constexpr std::uint64_t kMaximumResponseBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr std::uint64_t kPublishFlag = 1;

struct RequestHeader {
    std::array<char, 8> magic;
    std::uint64_t request_id;
    std::uint64_t event_size;
    std::uint64_t flags;
};

struct ResponseHeader {
    std::array<char, 8> magic;
    std::uint64_t request_id;
    std::uint32_t status;
    std::uint32_t metadata_size;
    std::uint64_t payload_size;
};

static_assert(sizeof(RequestHeader) == 32);
static_assert(sizeof(ResponseHeader) == 32);

std::runtime_error socket_error(const std::string& action) {
    return std::runtime_error(action + ": " + std::strerror(errno));
}

void wait_for(int socket, short events, std::chrono::steady_clock::time_point deadline) {
    while (true) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() < 0) {
            throw std::runtime_error("Gaudi sidecar request timed out");
        }
        pollfd descriptor{socket, events, 0};
        const auto timeout = static_cast<int>(std::min<std::int64_t>(
            remaining.count() + 1,
            std::numeric_limits<int>::max()));
        const int result = ::poll(&descriptor, 1, timeout);
        if (result > 0) {
            // A peer may close immediately after its final write, causing
            // POLLIN and POLLHUP to arrive together.  Consume the readable
            // bytes before treating the hangup as an error.
            if ((descriptor.revents & events) != 0) {
                return;
            }
            if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                throw std::runtime_error("Gaudi sidecar disconnected");
            }
            continue;
        }
        if (result == 0) {
            throw std::runtime_error("Gaudi sidecar request timed out");
        }
        if (errno != EINTR) {
            throw socket_error("Could not poll Gaudi sidecar socket");
        }
    }
}

void send_all(
    int socket,
    const void* source,
    std::size_t size,
    std::chrono::steady_clock::time_point deadline) {
    const auto* data = static_cast<const std::byte*>(source);
    while (size != 0) {
        wait_for(socket, POLLOUT, deadline);
        const auto sent = ::send(socket, data, size, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent > 0) {
            data += sent;
            size -= static_cast<std::size_t>(sent);
        } else if (sent < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw socket_error("Could not send event to Gaudi sidecar");
        }
    }
}

void receive_all(
    int socket,
    void* destination,
    std::size_t size,
    std::chrono::steady_clock::time_point deadline) {
    auto* data = static_cast<std::byte*>(destination);
    while (size != 0) {
        wait_for(socket, POLLIN, deadline);
        const auto received = ::recv(socket, data, size, MSG_DONTWAIT);
        if (received > 0) {
            data += received;
            size -= static_cast<std::size_t>(received);
        } else if (received == 0) {
            throw std::runtime_error("Gaudi sidecar closed the connection");
        } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw socket_error("Could not receive Gaudi sidecar response");
        }
    }
}

}  // namespace

GaudiClient::GaudiClient(
    std::string socket_path,
    std::chrono::milliseconds connect_timeout,
    std::chrono::milliseconds request_timeout)
    : socket_path_(std::move(socket_path)),
      connect_timeout_(connect_timeout),
      request_timeout_(request_timeout) {}

GaudiClient::~GaudiClient() {
    close();
}

void GaudiClient::close() noexcept {
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
}

void GaudiClient::connect() {
    close();
    if (socket_path_.size() >= sizeof(sockaddr_un::sun_path)) {
        throw std::invalid_argument("Gaudi sidecar socket path is too long");
    }
    socket_ = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (socket_ < 0) {
        throw socket_error("Could not create Gaudi sidecar socket");
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, socket_path_.c_str(), socket_path_.size() + 1);
    if (::connect(socket_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 &&
        errno != EINPROGRESS) {
        const auto error = socket_error("Could not connect to Gaudi sidecar");
        close();
        throw error;
    }
    try {
        wait_for(
            socket_,
            POLLOUT,
            std::chrono::steady_clock::now() + connect_timeout_);
        int connection_error = 0;
        socklen_t length = sizeof(connection_error);
        if (::getsockopt(socket_, SOL_SOCKET, SO_ERROR, &connection_error, &length) != 0) {
            throw socket_error("Could not inspect Gaudi sidecar connection");
        }
        if (connection_error != 0) {
            errno = connection_error;
            throw socket_error("Could not connect to Gaudi sidecar");
        }
    } catch (...) {
        close();
        throw;
    }
}

GaudiResponse GaudiClient::process(const std::byte* event, std::size_t size) {
    const GaudiEventRequest request{event, size, true};
    auto responses = process_batch({request});
    return std::move(responses.front());
}

std::vector<GaudiResponse> GaudiClient::process_batch(
    const std::vector<GaudiEventRequest>& events) {
    if (events.empty()) return {};
    for (const auto& event : events) {
        if (event.size != 0 && event.data == nullptr) {
            throw std::invalid_argument("Non-empty Gaudi request cannot have null event data");
        }
    }
    if (socket_ < 0) {
        connect();
    }
    const auto deadline = std::chrono::steady_clock::now() + request_timeout_;
    try {
        std::vector<GaudiResponse> results;
        results.reserve(events.size());
        for (const auto& event : events) {
            const auto request_id = next_request_id_++;
            const RequestHeader request{
                kRequestMagic, request_id, event.size, event.publish ? kPublishFlag : 0};
            const auto request_started = std::chrono::steady_clock::now();
            send_all(socket_, &request, sizeof(request), deadline);
            send_all(socket_, event.data, event.size, deadline);
            const auto request_sent = std::chrono::steady_clock::now();

            ResponseHeader response{};
            receive_all(socket_, &response, sizeof(response), deadline);
            const auto response_header_received = std::chrono::steady_clock::now();
            if (response.magic != kResponseMagic ||
                response.request_id != request_id) {
                throw std::runtime_error("Gaudi sidecar returned an invalid response header");
            }
            if (response.metadata_size > kMaximumResponseBytes ||
                response.payload_size > kMaximumResponseBytes ||
                response.metadata_size + response.payload_size > kMaximumResponseBytes) {
                throw std::runtime_error("Gaudi sidecar response exceeds the safety limit");
            }
            GaudiResponse result;
            result.metadata.resize(response.metadata_size);
            ByteBuffer payload(response.payload_size);
            receive_all(socket_, result.metadata.data(), result.metadata.size(), deadline);
            receive_all(socket_, payload.data(), payload.size(), deadline);
            const auto response_received = std::chrono::steady_clock::now();
            result.timing.request_send_nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    request_sent - request_started).count();
            result.timing.response_wait_nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    response_header_received - request_sent).count();
            result.timing.response_receive_nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    response_received - response_header_received).count();
            result.timing.round_trip_nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    response_received - request_started).count();
            if (response.status != 0) {
                throw std::runtime_error(
                    result.metadata.empty() ? "Gaudi sidecar rejected the event" : result.metadata);
            }
            const auto metadata = nlohmann::json::parse(result.metadata);
            const auto& products = metadata.at("products");
            if (!products.is_array()) {
                throw std::runtime_error("Gaudi sidecar product manifest is not an array");
            }
            result.products.reserve(products.size());
            for (const auto& entry : products) {
                const auto offset = entry.at("offset").get<std::size_t>();
                const auto size = entry.at("size").get<std::size_t>();
                if (offset > payload.size() || size > payload.size() - offset) {
                    throw std::runtime_error("Gaudi sidecar product exceeds its payload envelope");
                }
                ByteBuffer product_payload(size);
                std::copy_n(payload.begin() + static_cast<std::ptrdiff_t>(offset), size,
                            product_payload.begin());
                result.products.push_back({
                    std::move(product_payload),
                    entry.at("content-type").get<std::string>(),
                    entry.at("schema").get<std::string>(),
                });
            }
            results.push_back(std::move(result));
        }
        return results;
    } catch (...) {
        close();
        throw;
    }
}

}  // namespace zmq_publisher::gaudi
