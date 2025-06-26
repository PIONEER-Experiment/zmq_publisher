#ifndef MIDAS_EVENT_PROCESSOR_H
#define MIDAS_EVENT_PROCESSOR_H

#include "processors/GeneralProcessor.h"
#include "MidasReceiver.h"
#include "pipeline/pipeline.h"
#include "config/config_manager.h"
#include <chrono>
#include <nlohmann/json.hpp>

class MidasEventProcessor : public GeneralProcessor {
public:
    explicit MidasEventProcessor(int verbose = 0);
    ~MidasEventProcessor() override;

    void Init(const nlohmann::json& midas_receiver_config,
            const nlohmann::json& pipeline_config,
            const nlohmann::json& midas_event_processor_config);

    std::vector<std::string> getProcessedOutput() override;
    bool isReadyToProcess() const override;

private:
    MidasReceiver& midasReceiver_;
    std::chrono::system_clock::time_point lastEventTimestamp_;
    std::chrono::system_clock::time_point lastTransitionTimestamp_;
    bool initialized_ = false;
    size_t numEventsPerRetrieval_ = 1;
    INT lastRunNumber_ = -1;
    bool clearProductsOnNewRun_ = true;

    std::shared_ptr<ConfigManager> configManager_;
    std::unique_ptr<Pipeline> pipeline_;

    void handleTransitions();
    void setRunNumber(INT newRunNumber);
};

#endif // MIDAS_EVENT_PROCESSOR_H
