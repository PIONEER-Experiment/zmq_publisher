// MidasEventProcessor.cpp
#include "processors/MidasEventProcessor.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <spdlog/spdlog.h>
#include "analysis_pipeline/core/context/input_bundle.h"

using json = nlohmann::json;

MidasEventProcessor::MidasEventProcessor(int verbose)
    : GeneralProcessor(verbose),
      midasReceiver_(MidasReceiver::getInstance()),
      lastEventTimestamp_(std::chrono::system_clock::now()),
      lastTransitionTimestamp_(std::chrono::system_clock::now()) {}

MidasEventProcessor::~MidasEventProcessor() {
    if (initialized_) {
        midasReceiver_.stop();
    }
}

void MidasEventProcessor::Init(const json& midas_receiver_config,
                               const json& pipeline_config,
                               const json& midas_event_processor_config)
{
    if (!midas_receiver_config.is_object() || !pipeline_config.is_object() || !midas_event_processor_config.is_object()) {
        throw std::invalid_argument("[MidasEventProcessor] Init requires three JSON objects.");
    }

    MidasReceiverConfig config;
    config.host = midas_receiver_config.value("host", "");
    config.experiment = midas_receiver_config.value("experiment", "");
    config.bufferName = midas_receiver_config.value("buffer", "SYSTEM");
    config.clientName = midas_receiver_config.value("client-name", "MidasEventProcessor");
    config.eventID = midas_receiver_config.value("event-id", -1);
    config.getAllEvents = midas_receiver_config.value("get-all", true);
    config.maxBufferSize = midas_receiver_config.value("buffer-size", 1000);
    config.cmYieldTimeout = midas_receiver_config.value("yield-timeout-ms", 300);
    numEventsPerRetrieval_ = midas_receiver_config.value("num-events-per-retrieval", 1);

    config.transitionRegistrations = {
        {TR_START, 100}
    };

    midasReceiver_.init(config);
    midasReceiver_.start();

    configManager_ = std::make_shared<ConfigManager>();
    configManager_->reset();
    configManager_->addJsonObject(pipeline_config);
    if (!configManager_->validate()) {
        throw std::runtime_error("[MidasEventProcessor] Pipeline config validation failed.");
    }

    pipeline_ = std::make_unique<Pipeline>(configManager_);
    if (!pipeline_->buildFromConfig()) {
        throw std::runtime_error("[MidasEventProcessor] Failed to build pipeline.");
    }

    if (midas_event_processor_config.is_object()) {
        clearProductsOnNewRun_ = midas_event_processor_config.value("clear-products-on-new-run", true);

        if (midas_event_processor_config.contains("tags_to_omit_from_clear") &&
            midas_event_processor_config["tags_to_omit_from_clear"].is_array()) {
            for (const auto& tag : midas_event_processor_config["tags_to_omit_from_clear"]) {
                tagsToOmitFromClear_.insert(tag.get<std::string>());
            }
        }
    }

    initialized_ = true;
}

bool MidasEventProcessor::isReadyToProcess() const {
    return initialized_;
}

void MidasEventProcessor::handleTransitions() {
    auto transitions = midasReceiver_.getLatestTransitions(10, lastTransitionTimestamp_);

    for (const auto& t : transitions) {
        if (t.run_number != lastRunNumber_) {
            setRunNumber(t.run_number);
        }
    }

    if (!transitions.empty()) {
        lastTransitionTimestamp_ = transitions.back().timestamp;
    }
}

void MidasEventProcessor::setRunNumber(INT newRunNumber) {
    lastRunNumber_ = newRunNumber;

    if (verbose > 0) {
        spdlog::debug("[MidasEventProcessor] Run transition detected: run {}", newRunNumber);
    }

    if (clearProductsOnNewRun_) {
        spdlog::debug("[MidasEventProcessor] Clearing data products for new run.");
        auto& dataProductManager = pipeline_->getDataProductManager();

        if (tagsToOmitFromClear_.empty()) {
            dataProductManager.clear();
        } else {
            dataProductManager.removeExcludingTags(tagsToOmitFromClear_);
        }
    }
}

std::vector<std::string> MidasEventProcessor::getProcessedOutput() {
    std::vector<std::string> out;
    if (!initialized_) return out;

    handleTransitions();

    auto timedEvents = midasReceiver_.getLatestEvents(numEventsPerRetrieval_, lastEventTimestamp_);

    for (auto& timedEvent : timedEvents) {
        InputBundle input;

        input.set("TMEvent", timedEvent->event);
        input.set("timestamp", timedEvent->timestamp);
        input.set("run_number", lastRunNumber_);

        pipeline_->setInputData(std::move(input));
        pipeline_->execute();

        json serializedData = pipeline_->getDataProductManager().serializeAll();

        json outJson;
        outJson["run_number"] = lastRunNumber_;
        outJson["data_products"] = serializedData;

        out.push_back(outJson.dump());
    }

    if (!timedEvents.empty()) {
        lastEventTimestamp_ = timedEvents.back()->timestamp;
    }

    return out;
}
