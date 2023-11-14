#ifndef MDUMPPROCESSOR_H
#define MDUMPPROCESSOR_H

#include "CommandProcessor.h"
#include "EventProcessor.h"
#include "serializer/Serializer.hh"

class MdumpProcessor : public CommandProcessor {
public:
    MdumpProcessor(const std::string& detectorMappingFile, int verbose = 0, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;

private:
    void updateHistograms(std::vector<dataProducts::Waveform> waveforms);
    void doIntegration(dataProducts::WaveformCollection *input, dataProducts::WaveformIntegralCollection* output);
    std::string detectorMappingFile;
    EventProcessor eventProcessor;
    unpackers::Serializer* serializer;
};

#endif // MDUMPPROCESSOR_H
