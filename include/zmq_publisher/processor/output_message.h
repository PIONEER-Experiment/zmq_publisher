#pragma once

#include "zmq_publisher/processor/immutable_payload.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace zmq_publisher {

class OutputMessage {
public:
    struct TimingMarker {
        std::string stage;
        std::chrono::steady_clock::time_point timestamp;
    };

    OutputMessage(
        ByteBuffer payload,
        std::string content_type = "application/octet-stream",
        std::string schema = {});
    OutputMessage(
        ImmutablePayload payload,
        std::string content_type = "application/octet-stream",
        std::string schema = {});

    static OutputMessage from_text(
        std::string_view text,
        std::string content_type = "text/plain",
        std::string schema = {});

    const ImmutablePayload& payload() const noexcept { return payload_; }
    const std::string& content_type() const noexcept { return content_type_; }
    const std::string& schema() const noexcept { return schema_; }
    std::uint64_t sequence() const noexcept { return sequence_; }
    std::chrono::steady_clock::time_point created_at() const noexcept {
        return created_at_;
    }
    const std::vector<TimingMarker>& timing_markers() const noexcept {
        return timing_markers_;
    }
    const std::unordered_map<std::string, std::int64_t>& numeric_attributes() const noexcept {
        return numeric_attributes_;
    }
    std::size_t size() const noexcept { return payload_.size(); }

    void set_sequence(std::uint64_t sequence) noexcept { sequence_ = sequence; }
    void add_timing_marker(
        std::string stage,
        std::chrono::steady_clock::time_point timestamp);
    void set_numeric_attribute(std::string name, std::int64_t value);

private:
    ImmutablePayload payload_;
    std::string content_type_;
    std::string schema_;
    std::uint64_t sequence_ = 0;
    std::chrono::steady_clock::time_point created_at_ =
        std::chrono::steady_clock::now();
    std::vector<TimingMarker> timing_markers_;
    std::unordered_map<std::string, std::int64_t> numeric_attributes_;
};

}  // namespace zmq_publisher
