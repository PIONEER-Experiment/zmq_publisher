#include "sampic_payload_generator.h"
#include "synthetic_signal.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace zmq_publisher::benchmarks {
namespace {

#pragma pack(push, 1)
struct HitHeader {
    std::int32_t fe_board_index;
    std::int32_t channel;
    std::int32_t hit_number;
    std::int32_t sampic_index;
    std::int32_t channel_index;
    std::int32_t data_size;
    std::int32_t inl_corrected;
    std::int32_t adc_corrected;
    std::int32_t residual_pedestal_corrected;
    std::int32_t cell_info;
    std::int32_t first_cell_physical_index;
};

struct HitScalars {
    std::int32_t raw_tot_value;
    float tot_value_ns;
    float amplitude;
    float baseline;
    float peak;
    float time_index;
    double time_instant_ns;
    float time_amplitude;
    double first_cell_timestamp_ns;
};

struct EventTiming {
    std::uint64_t fe_timestamp_ns;
    std::uint32_t nhits;
    std::uint32_t nparents;
    std::uint32_t sp_prepare_us_sum;
    std::uint32_t sp_read_us_sum;
    std::uint32_t sp_decode_us_sum;
    std::uint32_t sp_total_us_sum;
    std::uint32_t sp_prepare_us_max;
    std::uint32_t sp_read_us_max;
    std::uint32_t sp_decode_us_max;
    std::uint32_t sp_total_us_max;
    std::uint32_t sp_acq_retry_max;
    std::uint32_t sp_acq_retry_sum;
};

struct CollectorTiming {
    std::uint64_t collector_timestamp_ns;
    std::uint32_t n_events;
    std::uint32_t total_hits;
    std::uint32_t wait_us;
    std::uint32_t group_build_us;
    std::uint32_t finalize_us;
    std::uint32_t total_us;
};

struct HitMetadataHeader {
    std::uint16_t format_version;
    std::uint16_t record_size;
    std::uint32_t hit_count;
};

struct HitMetadata {
    std::uint16_t sampic_data_header;
    std::int32_t first_trigger_position_cell;
    std::int32_t trigger_position_cell;
    double physical_cell0_timestamp_ns;
    std::int32_t sampic_timestamp_a;
    std::int32_t sampic_timestamp_b;
    std::uint64_t fpga_timestamp;
    std::int32_t adc_counter_latched_at_end_of_conversion;
    std::int32_t start_of_adc_ramp;
    std::uint64_t trigger_position_mask;
    std::int32_t time_physical_index;
};

struct TriggerGroup {
    std::uint32_t fpga_trigger_id;
    std::uint32_t external_trigger_id;
    std::uint32_t trigger_index_in_sampic_event;
    std::uint32_t assigned_hits;
    std::uint32_t ambiguous_hits;
    double trigger_timestamp_ns;
    double hit_reference_timestamp_ns;
};

struct VendorTrigger {
    std::uint32_t fpga_trigger_id;
    std::uint32_t external_trigger_id;
    std::uint16_t spill_number;
    std::uint16_t raw_extra_word;
    double trigger_timestamp_ns;
};
#pragma pack(pop)

static_assert(sizeof(HitHeader) + 64 * sizeof(float) + sizeof(HitScalars) == 344);
static_assert(sizeof(EventTiming) == 56);
static_assert(sizeof(CollectorTiming) == 32);
static_assert(sizeof(HitMetadata) == 54);
static_assert(sizeof(TriggerGroup) == 36);
static_assert(sizeof(VendorTrigger) == 20);

template <typename Value>
void append(std::vector<std::byte>& output, const Value& value) {
    const auto old_size = output.size();
    output.resize(old_size + sizeof(Value));
    std::memcpy(output.data() + old_size, &value, sizeof(Value));
}

SampicSimulationMode mode_from_environment() {
    const char* value = std::getenv("ZMQ_RATE_TEST_SAMPIC_MODE");
    if (value == nullptr || std::string_view(value) == "time_grouping") {
        return SampicSimulationMode::time_grouping;
    }
    const std::string_view mode(value);
    if (mode == "default") return SampicSimulationMode::default_grouping;
    if (mode == "external_gated_trigger") return SampicSimulationMode::external_gated_trigger;
    if (mode == "vendor_passthrough") return SampicSimulationMode::vendor_passthrough;
    throw std::invalid_argument(
        "ZMQ_RATE_TEST_SAMPIC_MODE must be default, time_grouping, "
        "external_gated_trigger, or vendor_passthrough");
}

bool dqm_mode_from_environment() {
    const char* value = std::getenv("ZMQ_RATE_TEST_SAMPIC_DQM_MODE");
    return value != nullptr && std::string_view(value) != "0";
}

}  // namespace

SampicPayloadGenerator::SampicPayloadGenerator()
    : mode_(mode_from_environment()), online_dqm_mode_(dqm_mode_from_environment()) {}

SampicPayloadGenerator::SampicPayloadGenerator(SampicSimulationMode mode)
    : SampicPayloadGenerator(mode, false) {}

SampicPayloadGenerator::SampicPayloadGenerator(
    SampicSimulationMode mode, bool online_dqm_mode)
    : mode_(mode), online_dqm_mode_(online_dqm_mode) {}

std::string_view SampicPayloadGenerator::detector_name() const noexcept {
    return "sampic";
}

std::vector<DetectorBank> SampicPayloadGenerator::generate(
    std::size_t requested_payload_bytes,
    std::uint64_t sequence) const {
    constexpr std::size_t hit_bytes =
        sizeof(HitHeader) + 64 * sizeof(float) + sizeof(HitScalars);
    const auto maximum_hit_count =
        std::max<std::size_t>(1, requested_payload_bytes / hit_bytes);
    const auto minimum_hit_count = std::min(
        maximum_hit_count, std::max<std::size_t>(8, maximum_hit_count / 2));
    const auto sampled_hit_count = std::llround(
        0.75 * static_cast<double>(maximum_hit_count) +
        SyntheticSignal::noise(
            SyntheticSignal::seed(0x53414d504943ULL, sequence, 0),
            0.10 * static_cast<double>(maximum_hit_count)));
    const auto hit_count = online_dqm_mode_
        ? static_cast<std::size_t>(std::clamp<long long>(
              sampled_hit_count, minimum_hit_count, maximum_hit_count))
        : maximum_hit_count;

    DetectorBank data{{'S', 'D', '0', '0', '\0'}, {}};
    DetectorBank metadata{{'S', 'H', '0', '0', '\0'}, {}};
    data.bytes.reserve(hit_count * hit_bytes);
    if (mode_ != SampicSimulationMode::default_grouping) {
        metadata.bytes.reserve(sizeof(HitMetadataHeader) + hit_count * sizeof(HitMetadata));
        append(metadata.bytes, HitMetadataHeader{
            1,
            static_cast<std::uint16_t>(sizeof(HitMetadata)),
            static_cast<std::uint32_t>(hit_count),
        });
    }
    for (std::size_t index = 0; index < hit_count; ++index) {
        const auto layer = index % 8;
        const auto track = index / 8;
        const auto track_center = static_cast<int>((sequence * 5U + track * 11U) % 32U);
        const auto strip = online_dqm_mode_
            ? std::clamp(track_center + static_cast<int>(index % 5) - 2, 0, 31)
            : static_cast<int>(index % 32);
        const auto global_channel = static_cast<int>(layer * 32 + strip);
        const auto board = online_dqm_mode_ ? global_channel / 64 : 0;
        const auto local_channel = online_dqm_mode_ ? global_channel % 64
                                                    : static_cast<int>(index % 256);
        const HitHeader header{
            board,
            local_channel,
            static_cast<std::int32_t>(index),
            local_channel / 16,
            local_channel % 16,
            64,
            1,
            1,
            1,
            static_cast<std::int32_t>(index % 64),
            static_cast<std::int32_t>(index % 64),
        };
        append(data.bytes, header);
        std::array<float, 64> waveform{};
        const auto signal_seed = SyntheticSignal::seed(0x53414d504943ULL, sequence, index);
        const auto onset = 17.0 + SyntheticSignal::trigger_jitter(signal_seed, 1.25);
        const auto rise_time = 0.9 + 0.4 * SyntheticSignal::uniform(signal_seed + 1U);
        const auto decay_time = 6.0 + 2.5 * SyntheticSignal::uniform(signal_seed + 2U);
        const auto amplitude = online_dqm_mode_
            ? std::clamp(0.18 + SyntheticSignal::noise(signal_seed + 3U, 0.045), 0.06, 0.35)
            : 60.0 + 180.0 * SyntheticSignal::uniform(signal_seed + 3U);
        const auto baseline = online_dqm_mode_
            ? 0.70 + SyntheticSignal::noise(signal_seed + 4U, 0.012) : 0.0;
        for (std::size_t sample = 0; sample < waveform.size(); ++sample) {
            const auto noise = SyntheticSignal::noise(
                SyntheticSignal::seed(0x53414d504943ULL, sequence, index, sample),
                online_dqm_mode_ ? 0.004 : 3.0);
            const auto pulse = SyntheticSignal::double_exponential_pulse(
                sample, onset, rise_time, decay_time, amplitude);
            waveform[sample] = static_cast<float>(baseline + noise +
                (online_dqm_mode_ ? -pulse : pulse));
        }
        append(data.bytes, waveform);
        append(data.bytes, HitScalars{
            10, 1.0F, static_cast<float>(online_dqm_mode_ ? -amplitude : amplitude),
            static_cast<float>(baseline),
            static_cast<float>(online_dqm_mode_ ? baseline - amplitude : amplitude),
            static_cast<float>(onset),
            static_cast<double>(sequence),
            static_cast<float>(online_dqm_mode_ ? -amplitude : amplitude),
            static_cast<double>(sequence),
        });
        if (mode_ != SampicSimulationMode::default_grouping) {
            append(metadata.bytes, HitMetadata{
                0xA501,
                static_cast<std::int32_t>((index + 8) % 64),
                static_cast<std::int32_t>((index + 16) % 64),
                static_cast<double>(sequence) * 1000.0,
                static_cast<std::int32_t>(sequence & 0x7fffffffU),
                static_cast<std::int32_t>((sequence + index) & 0x7fffffffU),
                sequence * 64U + index,
                static_cast<std::int32_t>(1000 + index),
                static_cast<std::int32_t>(2000 + index),
                std::uint64_t{1} << (index % 64),
                static_cast<std::int32_t>(index % 64),
            });
        }
    }

    DetectorBank timing{{'S', 'T', '0', '0', '\0'}, {}};
    append(timing.bytes, EventTiming{
        sequence, static_cast<std::uint32_t>(hit_count), 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    });
    std::vector<DetectorBank> banks;
    banks.reserve(4);
    banks.emplace_back(std::move(data));
    if (mode_ != SampicSimulationMode::default_grouping) {
        banks.emplace_back(std::move(metadata));
    }
    banks.emplace_back(std::move(timing));

    if (mode_ == SampicSimulationMode::default_grouping ||
        mode_ == SampicSimulationMode::time_grouping) {
        DetectorBank collector{{'S', 'C', '0', '0', '\0'}, {}};
        append(collector.bytes, CollectorTiming{
            sequence * 1000U,
            1,
            static_cast<std::uint32_t>(hit_count),
            2,
            3,
            1,
            6,
        });
        banks.emplace_back(std::move(collector));
    } else if (mode_ == SampicSimulationMode::external_gated_trigger) {
        DetectorBank trigger_group{{'S', 'G', '0', '0', '\0'}, {}};
        append(trigger_group.bytes, TriggerGroup{
            static_cast<std::uint32_t>(sequence),
            static_cast<std::uint32_t>(sequence + 10'000U),
            0,
            static_cast<std::uint32_t>(hit_count),
            0,
            static_cast<double>(sequence) * 1000.0,
            static_cast<double>(sequence) * 1000.0 + 4.0,
        });
        banks.emplace_back(std::move(trigger_group));
    } else {
        DetectorBank vendor_triggers{{'S', 'V', '0', '0', '\0'}, {}};
        constexpr std::uint32_t trigger_count = 1;
        append(vendor_triggers.bytes, trigger_count);
        append(vendor_triggers.bytes, VendorTrigger{
            static_cast<std::uint32_t>(sequence),
            static_cast<std::uint32_t>(sequence + 10'000U),
            static_cast<std::uint16_t>(sequence % 65536U),
            0,
            static_cast<double>(sequence) * 1000.0,
        });
        banks.emplace_back(std::move(vendor_triggers));
    }
    return banks;
}

}  // namespace zmq_publisher::benchmarks
