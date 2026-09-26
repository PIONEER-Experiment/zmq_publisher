#include "zmq_publisher/processor/immutable_payload.h"

#include <utility>

namespace zmq_publisher {

ImmutablePayload::ImmutablePayload(ByteBuffer bytes)
    : owner_(std::make_shared<const ByteBuffer>(std::move(bytes))) {
    const auto& storage = *static_cast<const ByteBuffer*>(owner_.get());
    data_ = storage.data();
    size_ = storage.size();
}

ImmutablePayload::ImmutablePayload(
    std::shared_ptr<const void> owner,
    const std::byte* data,
    std::size_t size)
    : owner_(std::move(owner)), data_(data), size_(size) {
    if (!owner_) {
        throw std::invalid_argument("Payload owner cannot be null");
    }
    if (size_ != 0 && data_ == nullptr) {
        throw std::invalid_argument("Non-empty payload data cannot be null");
    }
}

}  // namespace zmq_publisher
