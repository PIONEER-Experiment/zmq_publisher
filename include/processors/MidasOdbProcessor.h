#ifndef MIDAS_ODB_PROCESSOR_H
#define MIDAS_ODB_PROCESSOR_H

#include "processors/GeneralProcessor.h"
#include "MidasReceiver.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <string>

class MidasOdbProcessor : public GeneralProcessor {
public:
    explicit MidasOdbProcessor(int verbose = 0);
    ~MidasOdbProcessor() override;

    void Init(const nlohmann::json& midas_receiver_config);
    std::vector<std::string> getProcessedOutput() override;
    bool isReadyToProcess() const override;

private:
    std::chrono::system_clock::time_point lastProcessedTime_ = std::chrono::system_clock::now();
    MidasReceiver& midasReceiver_;
    bool initialized_ = false;
};

#endif // MIDAS_ODB_PROCESSOR_H
