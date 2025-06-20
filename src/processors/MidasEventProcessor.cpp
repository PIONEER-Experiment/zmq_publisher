#include "processors/MidasEventProcessor.h"
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

MidasEventProcessor::MidasEventProcessor(int verbose)
    : GeneralProcessor(verbose),
      midasReceiver_(MidasReceiver::getInstance()),
      lastTimestamp_(std::chrono::system_clock::now()) {}

MidasEventProcessor::~MidasEventProcessor() {
    if (initialized_) {
        midasReceiver_.stop();
    }
    delete outputTree_;
}

void MidasEventProcessor::Init(const json& midas_receiver_config, const json& pipeline_config) {
    if (!midas_receiver_config.is_object() || !pipeline_config.is_object()) {
        throw std::invalid_argument("[MidasEventProcessor] Init requires two JSON objects.");
    }

    // MIDAS Receiver Config
    std::string host = midas_receiver_config.value("host", "");
    std::string experiment = midas_receiver_config.value("experiment", "");
    std::string buffer = midas_receiver_config.value("buffer", "SYSTEM");
    std::string clientName = midas_receiver_config.value("client-name", "MidasEventProcessor");
    int eventId = midas_receiver_config.value("event-id", -1);
    bool getAll = midas_receiver_config.value("get-all", true);
    int bufferSize = midas_receiver_config.value("buffer-size", 1000);
    int yieldTimeoutMs = midas_receiver_config.value("yield-timeout-ms", 300);
    numEventsPerRetrieval_ = midas_receiver_config.value("num-events-per-retrieval", 1);

    midasReceiver_.init(host, experiment, buffer, clientName, eventId, getAll, bufferSize, yieldTimeoutMs);
    midasReceiver_.start();

    // Pipeline Config
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

    // Setup output tree
    outputTree_ = new TTree("events", "MIDAS Processed Events");
    pipeline_->setTree(outputTree_);

    initialized_ = true;
}

bool MidasEventProcessor::isReadyToProcess() const {
    return initialized_;
}

std::vector<std::string> MidasEventProcessor::getProcessedOutput() {
    std::vector<std::string> out;
    if (!initialized_ || outputTree_ == nullptr) return out;

    auto timedEvents = midasReceiver_.getLatestEvents(numEventsPerRetrieval_, lastTimestamp_);
    for (const auto& timedEvent : timedEvents) {
        pipeline_->setCurrentEvent(timedEvent.event);
        pipeline_->execute();
    }
    if (!timedEvents.empty()) {
        lastTimestamp_ = timedEvents.back().timestamp;
    }

    TTreeReader reader(outputTree_);

    TObjArray* branches = outputTree_->GetListOfBranches();
    int nBranches = branches->GetEntries();

    // Create a vector to hold TTreeReaderValue pointers for each branch
    std::vector<std::unique_ptr<TTreeReaderValue<std::string>>> readers;
    std::vector<std::string> branchNames;

    for (int b = 0; b < nBranches; ++b) {
        TBranch* branch = (TBranch*)branches->At(b);
        const char* branchName = branch->GetName();

        branchNames.push_back(branchName);
        readers.emplace_back(std::make_unique<TTreeReaderValue<std::string>>(reader, branchName));
    }

    while (reader.Next()) {
        nlohmann::json event_json;
        for (size_t i = 0; i < readers.size(); ++i) {
            event_json[branchNames[i]] = *(*readers[i]);  // Dereference twice
        }
        out.push_back(event_json.dump());
    }


    outputTree_->Reset();
    return out;
}

