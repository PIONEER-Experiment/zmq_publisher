#pragma once

#include "detector_payload_generator.h"

namespace zmq_publisher::benchmarks {

class MusipPayloadGenerator final : public DetectorPayloadGenerator {
public:
    [[nodiscard]] std::string_view detector_name() const noexcept override;
    [[nodiscard]] std::vector<DetectorBank> generate(
        std::size_t requested_payload_bytes,
        std::uint64_t sequence) const override;
};

}  // namespace zmq_publisher::benchmarks
