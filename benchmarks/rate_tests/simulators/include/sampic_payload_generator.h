#pragma once

#include "detector_payload_generator.h"

namespace zmq_publisher::benchmarks {

enum class SampicSimulationMode {
    default_grouping,
    time_grouping,
    external_gated_trigger,
    vendor_passthrough,
};

class SampicPayloadGenerator final : public DetectorPayloadGenerator {
public:
    SampicPayloadGenerator();
    explicit SampicPayloadGenerator(SampicSimulationMode mode);
    SampicPayloadGenerator(SampicSimulationMode mode, bool online_dqm_mode);

    [[nodiscard]] std::string_view detector_name() const noexcept override;
    [[nodiscard]] std::vector<DetectorBank> generate(
        std::size_t requested_payload_bytes,
        std::uint64_t sequence) const override;

private:
    SampicSimulationMode mode_;
    bool online_dqm_mode_;
};

}  // namespace zmq_publisher::benchmarks
