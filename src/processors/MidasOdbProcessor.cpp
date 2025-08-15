#include "processors/MidasOdbProcessor.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

MidasOdbProcessor::MidasOdbProcessor(int verbose)
    : GeneralProcessor(verbose),
      midasReceiver_(MidasReceiver::getInstance()) {}

MidasOdbProcessor::~MidasOdbProcessor() {
    if (initialized_) {
        midasReceiver_.stop();
    }
}

void MidasOdbProcessor::Init(const json& midas_receiver_config) {
    if (!midas_receiver_config.is_object()) {
        throw std::invalid_argument("[MidasOdbProcessor] Init requires a JSON object for MIDAS receiver config.");
    }

    MidasReceiverConfig config;
    config.host = midas_receiver_config.value("host", "");
    config.experiment = midas_receiver_config.value("experiment", "");
    config.bufferName = midas_receiver_config.value("buffer", "SYSTEM");
    config.clientName = midas_receiver_config.value("client-name", "MidasOdbProcessor");
    config.cmYieldTimeout = midas_receiver_config.value("yield-timeout-ms", 300);

    if (!midasReceiver_.IsInitialized()) {
        midasReceiver_.init(config);
    }
    midasReceiver_.start();

    initialized_ = true;
}

bool MidasOdbProcessor::isReadyToProcess() const {
    if (!initialized_) return false;

    auto now = std::chrono::system_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProcessedTime_).count();

    return elapsedMs >= period;
}

std::vector<std::string> MidasOdbProcessor::getProcessedOutput() {
    std::vector<std::string> out;
    if (!initialized_) return out;

    try {
        std::string odbJson = midasReceiver_.getOdb("/");
        out.push_back(odbJson);
    } catch (const std::exception& e) {
        spdlog::error("[MidasOdbProcessor] Failed to retrieve ODB: {}", e.what());
    }

    return out;
}
