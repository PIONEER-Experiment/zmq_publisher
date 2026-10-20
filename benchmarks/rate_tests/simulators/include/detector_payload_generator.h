#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace zmq_publisher::benchmarks {

struct DetectorBank {
    std::array<char, 5> name{};
    std::vector<std::byte> bytes;
};

class DetectorPayloadGenerator {
public:
    virtual ~DetectorPayloadGenerator() = default;

    [[nodiscard]] virtual std::string_view detector_name() const noexcept = 0;
    [[nodiscard]] virtual std::vector<DetectorBank> generate(
        std::size_t requested_payload_bytes,
        std::uint64_t sequence) const = 0;
};

std::unique_ptr<DetectorPayloadGenerator> make_detector_payload_generator();

}  // namespace zmq_publisher::benchmarks

