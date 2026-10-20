#include "detector_payload_generator.h"

#include "hdsoc_payload_generator.h"
#include "musip_payload_generator.h"
#include "sampic_payload_generator.h"
#include "wfd5_payload_generator.h"

#include <stdexcept>

namespace zmq_publisher::benchmarks {

std::unique_ptr<DetectorPayloadGenerator> make_detector_payload_generator() {
#if defined(PIONEER_SIMULATOR_SAMPIC)
    return std::make_unique<SampicPayloadGenerator>();
#elif defined(PIONEER_SIMULATOR_MUSIP)
    return std::make_unique<MusipPayloadGenerator>();
#elif defined(PIONEER_SIMULATOR_HDSOC)
    return std::make_unique<HdsocPayloadGenerator>();
#elif defined(PIONEER_SIMULATOR_WFD5)
    return std::make_unique<Wfd5PayloadGenerator>();
#else
    throw std::logic_error("Detector simulator target has no detector selection");
#endif
}

}  // namespace zmq_publisher::benchmarks
