#ifndef HISTOGRAM_PROCESSOR_H
#define HISTOGRAM_PROCESSOR_H

#include "GeneralProcessor.h"
#include <string>
#include <vector>
#include <chrono>

class HistogramProcessor : public GeneralProcessor {
public:
    HistogramProcessor(int verbose = 0);
    std::vector<std::string> getProcessedOutput() override;
    bool isReadyToProcess() const override;
    int getPeriod() const override;
    void setPeriod(int newPeriod) override; 
    ~HistogramProcessor() override;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPublishTime;
};

#endif // HISTOGRAM_PROCESSOR_H
