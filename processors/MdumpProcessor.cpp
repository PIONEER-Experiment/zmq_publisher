#include "MdumpProcessor.h"
#include "ProjectPrinter.h"
#include "MdumpPackage.h"
#include "MidasEvent.h"
#include "MidasBank.h"
#include "dataProducts/DataProduct.hh"
#include "dataProducts/Waveform.hh"
#include "dataProducts/WaveformIntegral.hh"
#include "dataProducts/HodoscopeEvent.hh"
#include "serializer/Serializer.hh"
#include "HistogramStorage.h"
#include <iostream>

MdumpProcessor::MdumpProcessor(const std::string& detectorMappingFile, int verbose, const CommandRunner& runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile), eventProcessor(detectorMappingFile, verbose), serializer(new unpackers::Serializer(detectorMappingFile, 0, 0, 0)) {
}

std::vector<std::string> MdumpProcessor::getProcessedOutput() {
    ProjectPrinter printer;

    // Placeholder result
    std::vector<std::string> result;

    MdumpPackage mdumpPackage(commandRunner.execute());
    for (const MidasEvent& event : mdumpPackage.getEvents()) {
        if (eventProcessor.processEvent(event, "CR00") == 0) {
            // Serialize the event data with EventProcessor and store it in serializedData
            std::vector<std::string> serializedData = eventProcessor.getSerializedData();
            updateHistograms(eventProcessor.getWaveforms());
            result.insert(result.end(),serializedData.begin(),serializedData.end());
        }
    }

    return result;
}


void MdumpProcessor::updateHistograms(std::vector<dataProducts::Waveform> waveforms) {
    ProjectPrinter printer;

    // Clear and initialize collections
    dataProducts::JitterCorrectedWaveformCollection waveforms_jitter_corrected;
    dataProducts::WaveformIntegralCollection waveform_integrals;
    dataProducts::LYSOWaveformCollection lyso_waveforms;
    dataProducts::NaIWaveformCollection nai_waveforms;
    dataProducts::T0WaveformCollection t0_waveforms;
    dataProducts::HodoscopeWaveformCollection hodo_x_waveforms;
    dataProducts::HodoscopeWaveformCollection hodo_y_waveforms;
    dataProducts::WaveformCollection default_waveforms;
    dataProducts::WaveformIntegralCollection lyso_waveform_integrals;
    dataProducts::WaveformIntegralCollection nai_waveform_integrals;
    dataProducts::WaveformIntegralCollection t0_waveform_integrals;
    dataProducts::WaveformIntegralCollection hodo_x_waveform_integrals;
    dataProducts::WaveformIntegralCollection hodo_y_waveform_integrals;
    dataProducts::WaveformIntegralCollection default_waveform_integrals;

    //Do Jitter Correction
    applyJitterCorrection(waveforms_jitter_corrected, waveforms);
    
    // Perform integration
    doIntegration(&waveforms_jitter_corrected, &waveform_integrals);
    
    // Update energy histograms
    updateEnergyHistograms(waveform_integrals);
    
    // Separate waveforms based on detector type
    categorizeWaveforms(waveforms_jitter_corrected, lyso_waveforms, nai_waveforms, t0_waveforms, hodo_x_waveforms, hodo_y_waveforms, default_waveforms);
    
    //Perform integration on hodoscope waveforms
    // Note this is inefficient because we've already done these integrals,
    // But it won't matter that much in terms of efficiency
    doIntegration(&hodo_x_waveforms, &hodo_x_waveform_integrals);
    doIntegration(&hodo_y_waveforms, &hodo_y_waveform_integrals);

    // Process hodoscope integrals into a HodoscopeEvent object and update histogram
    updateXYHodoscopeHistograms(hodo_x_waveform_integrals,hodo_y_waveform_integrals);

}


void MdumpProcessor::doIntegration(dataProducts::WaveformCollection *input, dataProducts::WaveformIntegralCollection* output) {
    // perform pulse integration on an input vector and push the results back to an output vector
    output->clear();
    for (dataProducts::Waveform &w : *input) {
        //if(std::abs(w.PeakToPeak()) > (serializer->GetIntegralSizeCutoff(&w)) ) {
            output->push_back(dataProducts::WaveformIntegral(&w, serializer->GetNSigmaIntegration(&w) ,1));
        //}
    }
};


void MdumpProcessor::updateEnergyHistograms(dataProducts::WaveformIntegralCollection& waveform_integrals) {
    HistogramStorage& histogramStorage = HistogramStorage::getInstance();

    for (dataProducts::WaveformIntegral& waveform_integral : waveform_integrals) {
        dataProducts::ChannelID channelId = waveform_integral.GetID();
        std::string detectorName = serializer->GetDetectorType(channelId);
        int detectorNum = serializer->GetDetectorNum(channelId);
        std::string key = detectorName + "_" + std::to_string(detectorNum);
        histogramStorage.addToHistogram(key, waveform_integral.integral);
    }
}

void MdumpProcessor::categorizeWaveforms(dataProducts::JitterCorrectedWaveformCollection& waveforms_jitter_corrected,
                                         dataProducts::LYSOWaveformCollection& lyso_waveforms,
                                         dataProducts::NaIWaveformCollection& nai_waveforms,
                                         dataProducts::T0WaveformCollection& t0_waveforms,
                                         dataProducts::HodoscopeWaveformCollection& hodo_x_waveforms,
                                         dataProducts::HodoscopeWaveformCollection& hodo_y_waveforms,
                                         dataProducts::WaveformCollection& default_waveforms) {
    std::map<std::string,int> detector_name_map = { //TODO: put in config file
                    {"lyso",0},
                    {"nai",1},
                    {"t0",2},
                    {"hodo_x",3},
                    {"hodo_y",4},
                };
    for (const dataProducts::Waveform &w : waveforms_jitter_corrected) {
        std::string dettype = serializer->GetDetectorType(dataProducts::GetID(&w));
        switch (detector_name_map[dettype]) {
            case 0:
                lyso_waveforms.push_back(w);
                break;
            case 1:
                nai_waveforms.push_back(w);
                break;
            case 2:
                t0_waveforms.push_back(w);
                break;
            case 3:
                hodo_x_waveforms.push_back(w);
                break;
            case 4:
                hodo_y_waveforms.push_back(w);
                break;
            default:
                default_waveforms.push_back(w);
                break;
        }
    }
}

void MdumpProcessor::applyJitterCorrection(dataProducts::JitterCorrectedWaveformCollection& waveforms_jitter_corrected,
                                           std::vector<dataProducts::Waveform>& waveforms) {
    // Resize the collection to match the size of waveforms
    waveforms_jitter_corrected.resize(waveforms.size());

    //Apply jitter correction
    for (unsigned int i = 0; i < waveforms.size(); i++) {
        waveforms_jitter_corrected.at(i) = dataProducts::Waveform(&(waveforms.at(i)));
        waveforms_jitter_corrected.at(i).JitterCorrect(serializer->GetJitterCorrection(&(waveforms.at(i))));
        waveforms_jitter_corrected.at(i).CalculatePedestal(15); 
    }
}

void MdumpProcessor::updateXYHodoscopeHistograms(dataProducts::WaveformIntegralCollection& hodo_x_waveform_integrals,
                                                 dataProducts::WaveformIntegralCollection& hodo_y_waveform_integrals) {
    HistogramStorage& histogramStorage = HistogramStorage::getInstance();

    // Process hodoscope integrals into a HodoscopeEvent object
    if (hodo_x_waveform_integrals.size() > 0 || hodo_y_waveform_integrals.size() > 0) {
        dataProducts::HodoscopeEvent hodoscopeEvent(&hodo_x_waveform_integrals, &hodo_y_waveform_integrals);
        histogramStorage.addToHistogram("XY_hodoscope", hodoscopeEvent.max_channel_x, hodoscopeEvent.max_channel_y);
    }
}

