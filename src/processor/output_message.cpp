#include "zmq_publisher/processor/output_message.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace zmq_publisher {

OutputMessage::OutputMessage(
    ByteBuffer payload,
    std::string content_type,
    std::string schema)
    : payload_(std::make_shared<const ByteBuffer>(std::move(payload))),
      content_type_(std::move(content_type)),
      schema_(std::move(schema)) {
    if (content_type_.empty()) {
        throw std::invalid_argument("Output content type cannot be empty");
    }
}

OutputMessage OutputMessage::from_text(
    std::string_view text,
    std::string content_type,
    std::string schema) {
    ByteBuffer payload(text.size());
    std::transform(text.begin(), text.end(), payload.begin(), [](char value) {
        return static_cast<std::byte>(static_cast<unsigned char>(value));
    });
    return OutputMessage(std::move(payload), std::move(content_type), std::move(schema));
}

}  // namespace zmq_publisher
