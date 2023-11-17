#ifndef MDUMPPROCESSOR_H
#define MDUMPPROCESSOR_H

#include "CommandProcessor.h"
#include "EventProcessor.h"
#include "serializer/Serializer.hh"
#include "MidasConnector.h"
#include "MdumpDatabase.h"

class MdumpProcessor : public CommandProcessor {
public:
    MdumpProcessor(const std::string& detectorMappingFile, int verbose = 0, bool use_multi_threading = false, int num_workers = 5, const CommandRunner& runner = CommandRunner(""));

    std::vector<std::string> getProcessedOutput() override;
    
    void setCommandRunner(const CommandRunner& runner) override;

private:
    bool initializeMidas(MidasConnector& midasConnector);
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
    MdumpDatabase mdumpDatabase;
    bool useMultiThreading;
    int numWorkers;
};

#endif // MDUMPPROCESSOR_H
