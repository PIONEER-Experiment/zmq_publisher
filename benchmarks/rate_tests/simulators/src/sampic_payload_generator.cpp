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

struct OnlineAtarHit {
    int layer;
    int strip;
    double time_nanoseconds;
    double charge_fraction;
};

std::vector<OnlineAtarHit> generate_online_atar_time_group(
    std::uint64_t sequence,
    std::size_t maximum_hits) {
    constexpr int layers = 8;
    constexpr int strips = 32;
    // One frontend/MIDAS event represents one completed hardware time group.
    // Three consecutive groups share a generated decay-chain geometry so the
    // stream is realistic, but no online component needs to event-build them.
    const auto zero_based_sequence = sequence == 0 ? 0 : sequence - 1;
    const auto chain_sequence = zero_based_sequence / 3;
    const auto phase = static_cast<int>(zero_based_sequence % 3);
    const auto seed = SyntheticSignal::seed(0x41544152ULL, chain_sequence, 0);
    const auto timestamp_base = static_cast<double>(chain_sequence) * 10'000.0;
    const auto pion_stop_layer = 4 + static_cast<int>(3.0 * SyntheticSignal::uniform(seed + 1U));
    const auto pion_x0 = 10.0 + 12.0 * SyntheticSignal::uniform(seed + 2U);
    const auto pion_y0 = 10.0 + 12.0 * SyntheticSignal::uniform(seed + 3U);
    const auto pion_x_slope = SyntheticSignal::noise(seed + 4U, 0.22);
    const auto pion_y_slope = SyntheticSignal::noise(seed + 5U, 0.22);
    const auto pion_x_stop = pion_x0 + pion_x_slope * pion_stop_layer;
    const auto pion_y_stop = pion_y0 + pion_y_slope * pion_stop_layer;

    std::vector<OnlineAtarHit> hits;
    hits.reserve(maximum_hits);
    const auto add_track = [&](
        int first_layer,
        int last_layer,
        double x_at_first,
        double y_at_first,
        double x_slope,
        double y_slope,
        double group_time) {
        if (hits.size() >= maximum_hits) return;
        const auto direction = first_layer <= last_layer ? 1 : -1;
        for (auto layer = first_layer;; layer += direction) {
            const auto depth = static_cast<double>(layer - first_layer);
            const auto coordinate = layer % 2 == 0
                ? x_at_first + x_slope * depth
                : y_at_first + y_slope * depth;
            const auto central_strip = std::clamp(
                static_cast<int>(std::lround(coordinate)), 0, strips - 1);
            const auto neighbor_direction = coordinate >= central_strip ? 1 : -1;
            hits.push_back({
                layer,
                central_strip,
                timestamp_base + group_time +
                    std::abs(depth) * 0.8,
                1.0,
            });
            const auto neighbor_strip = std::clamp(
                central_strip + neighbor_direction, 0, strips - 1);
            if (hits.size() < maximum_hits && neighbor_strip != central_strip) {
                hits.push_back({
                    layer,
                    neighbor_strip,
                    timestamp_base + group_time +
                        std::abs(depth) * 0.8 + 0.15,
                    0.32,
                });
            }
            if (hits.size() >= maximum_hits || layer == last_layer) break;
        }
    };

    // Build a coherent chain geometry, then emit only this record's phase.
    const auto muon_direction = SyntheticSignal::uniform(seed + 6U) < 0.5 ? -1 : 1;
    const auto muon_stop_layer = std::clamp(
        pion_stop_layer + muon_direction *
            (1 + static_cast<int>(2.0 * SyntheticSignal::uniform(seed + 7U))),
        0,
        layers - 1);
    const auto muon_x_slope = SyntheticSignal::noise(seed + 8U, 0.65);
    const auto muon_y_slope = SyntheticSignal::noise(seed + 9U, 0.65);
    const auto muon_depth = static_cast<double>(muon_stop_layer - pion_stop_layer);
    const auto positron_x0 = pion_x_stop + muon_x_slope * muon_depth;
    const auto positron_y0 = pion_y_stop + muon_y_slope * muon_depth;
    const auto positron_exit_layer = SyntheticSignal::uniform(seed + 10U) < 0.5
        ? 0
        : layers - 1;
    const auto positron_time = 2'200.0 + SyntheticSignal::noise(seed + 13U, 180.0);

    if (phase == 0) {
        // An incoming pion ranges out in the target.
        add_track(0, pion_stop_layer, pion_x0, pion_y0,
                  pion_x_slope, pion_y_slope, 0.0);
    } else if (phase == 1) {
        // The short muon group starts at the generated pion stopping point.
        add_track(pion_stop_layer, muon_stop_layer, pion_x_stop, pion_y_stop,
                  muon_x_slope, muon_y_slope, 28.0);
    } else {
        // The later positron group starts at the generated muon stopping point.
        add_track(muon_stop_layer, positron_exit_layer, positron_x0, positron_y0,
                  SyntheticSignal::noise(seed + 11U, 1.25),
                  SyntheticSignal::noise(seed + 12U, 1.25), positron_time);
    }

    // Occasional coincident activity belongs to the same hardware time group.
    if (hits.size() < maximum_hits && SyntheticSignal::uniform(seed + 14U) < 0.12) {
        const auto group_time = phase == 0 ? 0.0 : (phase == 1 ? 28.0 : positron_time);
        add_track(0, layers - 1,
                  4.0 + 24.0 * SyntheticSignal::uniform(seed + 15U),
                  4.0 + 24.0 * SyntheticSignal::uniform(seed + 16U),
                  SyntheticSignal::noise(seed + 17U, 0.45),
                  SyntheticSignal::noise(seed + 18U, 0.45),
                  group_time);
    }
    if (hits.size() > maximum_hits) hits.resize(maximum_hits);
    return hits;
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
    const auto online_hits = online_dqm_mode_
        ? generate_online_atar_time_group(sequence, maximum_hit_count)
        : std::vector<OnlineAtarHit>{};
    const auto hit_count = online_dqm_mode_ ? online_hits.size() : maximum_hit_count;

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
        const auto layer = online_dqm_mode_ ? online_hits[index].layer : index % 8;
        const auto strip = online_dqm_mode_
            ? online_hits[index].strip
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
        const auto charge_sharing = online_dqm_mode_
            ? online_hits[index].charge_fraction
            : 1.0;
        const auto amplitude = online_dqm_mode_
            ? std::clamp(
                  charge_sharing *
                      (0.22 + SyntheticSignal::noise(signal_seed + 3U, 0.025)),
                  0.025,
                  0.35)
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
            online_dqm_mode_ ? online_hits[index].time_nanoseconds
                             : static_cast<double>(sequence),
            static_cast<float>(online_dqm_mode_ ? -amplitude : amplitude),
            online_dqm_mode_ ? online_hits[index].time_nanoseconds
                             : static_cast<double>(sequence),
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
