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
    void updateEnergyHistograms(dataProducts::WaveformIntegralCollection& waveform_integrals);
    void categorizeWaveforms(dataProducts::JitterCorrectedWaveformCollection& waveforms_jitter_corrected,
                             dataProducts::LYSOWaveformCollection& lyso_waveforms,
                             dataProducts::NaIWaveformCollection& nai_waveforms,
                             dataProducts::T0WaveformCollection& t0_waveforms,
                             dataProducts::HodoscopeWaveformCollection& hodo_x_waveforms,
                             dataProducts::HodoscopeWaveformCollection& hodo_y_waveforms,
                             dataProducts::WaveformCollection& default_waveforms);
    void applyJitterCorrection(dataProducts::JitterCorrectedWaveformCollection& waveforms_jitter_corrected,
                            std::vector<dataProducts::Waveform>& waveforms);
    void updateXYHodoscopeHistograms(dataProducts::WaveformIntegralCollection& hodo_x_waveform_integrals,
                                    dataProducts::WaveformIntegralCollection& hodo_y_waveform_integrals);

    std::string detectorMappingFile;
    EventProcessor eventProcessor;
    unpackers::Serializer* serializer;
};

#endif // MDUMPPROCESSOR_H
