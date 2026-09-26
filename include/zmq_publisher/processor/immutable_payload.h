#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace zmq_publisher {

using ByteBuffer = std::vector<std::byte>;

class ImmutablePayload {
public:
    explicit ImmutablePayload(ByteBuffer bytes);

    template <typename Element>
    static ImmutablePayload from_shared_vector(
        std::shared_ptr<const std::vector<Element>> storage) {
        static_assert(sizeof(Element) == 1, "Payload elements must be one byte wide");
        static_assert(std::is_trivially_copyable_v<Element>,
                      "Payload elements must be trivially copyable");
        if (!storage) {
            throw std::invalid_argument("Payload storage cannot be null");
        }
        const auto* data = reinterpret_cast<const std::byte*>(storage->data());
        const auto size = storage->size();
        std::shared_ptr<const void> owner(storage, storage.get());
        return ImmutablePayload(std::move(owner), data, size);
    }

    const std::byte* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

private:
    ImmutablePayload(
        std::shared_ptr<const void> owner,
        const std::byte* data,
        std::size_t size);

    std::shared_ptr<const void> owner_;
    const std::byte* data_ = nullptr;
    std::size_t size_ = 0;
};

}  // namespace zmq_publisher
