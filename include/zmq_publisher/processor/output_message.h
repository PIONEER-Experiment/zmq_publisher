#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace zmq_publisher {

using ByteBuffer = std::vector<std::byte>;

class OutputMessage {
public:
    OutputMessage(
        ByteBuffer payload,
        std::string content_type = "application/octet-stream",
        std::string schema = {});

    static OutputMessage from_text(
        std::string_view text,
        std::string content_type = "text/plain",
        std::string schema = {});

    const std::shared_ptr<const ByteBuffer>& payload() const noexcept { return payload_; }
    const std::string& content_type() const noexcept { return content_type_; }
    const std::string& schema() const noexcept { return schema_; }
    std::uint64_t sequence() const noexcept { return sequence_; }
    std::size_t size() const noexcept { return payload_->size(); }

    void set_sequence(std::uint64_t sequence) noexcept { sequence_ = sequence; }

private:
    std::shared_ptr<const ByteBuffer> payload_;
    std::string content_type_;
    std::string schema_;
    std::uint64_t sequence_ = 0;
};

}  // namespace zmq_publisher
