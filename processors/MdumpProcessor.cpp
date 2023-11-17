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
#include <chrono>

MdumpProcessor::MdumpProcessor(const std::string& detectorMappingFile, int verbose, bool use_multi_threading, int num_workers, const CommandRunner& runner)
    : CommandProcessor(verbose, runner), detectorMappingFile(detectorMappingFile), 
    eventProcessor(detectorMappingFile, verbose), 
    serializer(new unpackers::Serializer(detectorMappingFile, 0, 0, 0)), 
    mdumpDatabase(runner,verbose,num_workers),
    useMultiThreading(use_multi_threading) {
}


std::vector<std::string> MdumpProcessor::getProcessedOutput() {
    ProjectPrinter printer;

    // Placeholder result
    std::vector<std::string> result;

    // Start timing
    auto start = std::chrono::high_resolution_clock::now();

    MdumpPackage mdumpPackage;
    if (useMultiThreading) {
        mdumpDatabase.startWorker();
        if (!mdumpDatabase.hasNewData()) {
            return result;
        }
        mdumpPackage = MdumpPackage(mdumpDatabase.getLatestMdumpOutput());
    } else {
        mdumpPackage = MdumpPackage(commandRunner.execute());
    }

    // Measure the time taken by the commandRunner.execute()
    auto endCommandRunner = std::chrono::high_resolution_clock::now();
    auto durationCommandRunner = std::chrono::duration_cast<std::chrono::milliseconds>(endCommandRunner - start);
    printer.Print("CommandRunner.execute() took " + std::to_string(durationCommandRunner.count()) + " milliseconds");

    for (const MidasEvent& event : mdumpPackage.getEvents()) {
        auto startProcessEvent = std::chrono::high_resolution_clock::now();
        if (eventProcessor.processEvent(event, "CR00") == 0) {
            // Serialize the event data with EventProcessor and store it in serializedData
            std::vector<std::string> serializedData = eventProcessor.getSerializedData();
            auto startUpdateHistograms = std::chrono::high_resolution_clock::now();
            updateHistograms(eventProcessor.getWaveforms());
            auto endUpdateHistograms = std::chrono::high_resolution_clock::now();
            auto durationUpdateHistograms = std::chrono::duration_cast<std::chrono::milliseconds>(endUpdateHistograms - startUpdateHistograms);
            printer.Print("updateHistograms() took " + std::to_string(durationUpdateHistograms.count()) + " milliseconds");

            result.insert(result.end(), serializedData.begin(), serializedData.end());
        }
        auto endProcessEvent = std::chrono::high_resolution_clock::now();
        auto durationProcessEvent = std::chrono::duration_cast<std::chrono::milliseconds>(endProcessEvent - startProcessEvent);
        printer.Print("eventProcessor.processEvent() took " + std::to_string(durationProcessEvent.count()) + " milliseconds");
    }

    // Measure the overall time taken by the entire process
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printer.Print("Total processing time: " + std::to_string(duration.count()) + " milliseconds");

    return result;
}


/*
std::vector<std::string> MdumpProcessor::getProcessedOutput() {
    ProjectPrinter printer;
    // Placeholder result
    std::vector<std::string> result;

    INT max_event_size = 20000;
    // Allocate memory for storing event data dynamically
    void* event_data = malloc(max_event_size);

    MidasConnector midasConnector("Publisher");
        if (!initializeMidas(midasConnector)) {
            printer.PrintError("Failed to initialize MIDAS.", __LINE__, __FILE__);
            return result;
        }
    auto start = std::chrono::high_resolution_clock::now();
    int success = midasConnector.ReceiveEvent(event_data,max_event_size)
    if (success == 1) {
        eventProcessor.processEvent(event_data, max_event_size);
        result.push_back(eventProcessor.getSerializedData());

        auto endCommandRunner = std::chrono::high_resolution_clock::now();
        auto durationCommandRunner = std::chrono::duration_cast<std::chrono::milliseconds>(endCommandRunner - start);
        printer.Print("ReceiveEvent took " + std::to_string(durationCommandRunner.count()) + " milliseconds");

        auto startUpdateHistograms = std::chrono::high_resolution_clock::now();
        updateHistograms(eventProcessor.getWaveforms());
        auto endUpdateHistograms = std::chrono::high_resolution_clock::now();
        auto durationUpdateHistograms = std::chrono::duration_cast<std::chrono::milliseconds>(endUpdateHistograms - startUpdateHistograms);
        printer.Print("updateHistograms() took " + std::to_string(durationUpdateHistograms.count()) + " milliseconds");
    }

    // Measure the overall time taken by the entire process
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printer.Print("Total processing time: " + std::to_string(duration.count()) + " milliseconds");


    return result;
}
*/

void MdumpProcessor::setCommandRunner(const CommandRunner& runner) {
    commandRunner = runner;
    mdumpDatabase.setCommandRunner(runner);
}

#include <chrono>

void MdumpProcessor::updateHistograms(std::vector<dataProducts::Waveform> waveforms) {
    ProjectPrinter printer;
    std::pair<double, double> posi;

    // Start timing for GetPosition and Update Positions
    auto startGetPosition = std::chrono::high_resolution_clock::now();

    for (auto &wf : waveforms) {
        posi = serializer->GetPosition(wf.GetID());
        wf.x = posi.first;
        wf.y = posi.second;
    }

    // Stop timing for GetPosition and Update Positions and print the duration
    auto endGetPosition = std::chrono::high_resolution_clock::now();
    auto durationGetPosition = std::chrono::duration_cast<std::chrono::milliseconds>(endGetPosition - startGetPosition);
    printer.Print("GetPosition and Update Positions took " + std::to_string(durationGetPosition.count()) + " milliseconds");

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

    // Start timing for Jitter Correction
    auto startJitterCorrection = std::chrono::high_resolution_clock::now();
    applyJitterCorrection(waveforms_jitter_corrected, waveforms);
    // Stop timing for Jitter Correction and print the duration
    auto endJitterCorrection = std::chrono::high_resolution_clock::now();
    auto durationJitterCorrection = std::chrono::duration_cast<std::chrono::milliseconds>(endJitterCorrection - startJitterCorrection);
    printer.Print("Jitter Correction took " + std::to_string(durationJitterCorrection.count()) + " milliseconds");

    // Start timing for Integration
    auto startIntegration = std::chrono::high_resolution_clock::now();
    doIntegration(&waveforms_jitter_corrected, &waveform_integrals);
    // Stop timing for Integration and print the duration
    auto endIntegration = std::chrono::high_resolution_clock::now();
    auto durationIntegration = std::chrono::duration_cast<std::chrono::milliseconds>(endIntegration - startIntegration);
    printer.Print("Integration took " + std::to_string(durationIntegration.count()) + " milliseconds");

    // Start timing for Update Energy Histograms
    auto startUpdateEnergyHistograms = std::chrono::high_resolution_clock::now();
    updateEnergyHistograms(waveform_integrals);
    // Stop timing for Update Energy Histograms and print the duration
    auto endUpdateEnergyHistograms = std::chrono::high_resolution_clock::now();
    auto durationUpdateEnergyHistograms = std::chrono::duration_cast<std::chrono::milliseconds>(endUpdateEnergyHistograms - startUpdateEnergyHistograms);
    printer.Print("Update Energy Histograms took " + std::to_string(durationUpdateEnergyHistograms.count()) + " milliseconds");

    // Start timing for Categorize Waveforms
    auto startCategorizeWaveforms = std::chrono::high_resolution_clock::now();
    categorizeWaveforms(waveforms_jitter_corrected, lyso_waveforms, nai_waveforms, t0_waveforms, hodo_x_waveforms, hodo_y_waveforms, default_waveforms);
    // Stop timing for Categorize Waveforms and print the duration
    auto endCategorizeWaveforms = std::chrono::high_resolution_clock::now();
    auto durationCategorizeWaveforms = std::chrono::duration_cast<std::chrono::milliseconds>(endCategorizeWaveforms - startCategorizeWaveforms);
    printer.Print("Categorize Waveforms took " + std::to_string(durationCategorizeWaveforms.count()) + " milliseconds");

    // Start timing for Integration on Hodoscope Waveforms
    auto startIntegrationHodoX = std::chrono::high_resolution_clock::now();
    doIntegration(&hodo_x_waveforms, &hodo_x_waveform_integrals);
    // Stop timing for Integration on Hodoscope Waveforms and print the duration
    auto endIntegrationHodoX = std::chrono::high_resolution_clock::now();
    auto durationIntegrationHodoX = std::chrono::duration_cast<std::chrono::milliseconds>(endIntegrationHodoX - startIntegrationHodoX);
    printer.Print("Integration on Hodo X Waveforms took " + std::to_string(durationIntegrationHodoX.count()) + " milliseconds");

    // Start timing for Integration on Hodoscope Y Waveforms
    auto startIntegrationHodoY = std::chrono::high_resolution_clock::now();
    doIntegration(&hodo_y_waveforms, &hodo_y_waveform_integrals);
    // Stop timing for Integration on Hodoscope Y Waveforms and print the duration
    auto endIntegrationHodoY = std::chrono::high_resolution_clock::now();
    auto durationIntegrationHodoY = std::chrono::duration_cast<std::chrono::milliseconds>(endIntegrationHodoY - startIntegrationHodoY);
    printer.Print("Integration on Hodo Y Waveforms took " + std::to_string(durationIntegrationHodoY.count()) + " milliseconds");

    // Start timing for Update XY Hodoscope Histograms
    auto startUpdateXYHodoscopeHistograms = std::chrono::high_resolution_clock::now();
    updateXYHodoscopeHistograms(hodo_x_waveform_integrals, hodo_y_waveform_integrals);
    // Stop timing for Update XY Hodoscope Histograms and print the duration
    auto endUpdateXYHodoscopeHistograms = std::chrono::high_resolution_clock::now();
    auto durationUpdateXYHodoscopeHistograms = std::chrono::duration_cast<std::chrono::milliseconds>(endUpdateXYHodoscopeHistograms - startUpdateXYHodoscopeHistograms);
    printer.Print("Update XY Hodoscope Histograms took " + std::to_string(durationUpdateXYHodoscopeHistograms.count()) + " milliseconds");
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
        histogramStorage.addToHistogram("XY_hodoscope", hodoscopeEvent.max_x, hodoscopeEvent.max_y);
    }
}

bool MdumpProcessor::initializeMidas(MidasConnector& midasConnector) {
    // Set the MidasConnector properties based on the config
    midasConnector.setEventId(-1);
    midasConnector.setTriggerMask(-1);
    midasConnector.setSamplingType(2);
    midasConnector.setBufferSize(33554432);
    midasConnector.setBufferName("SYSTEM");
    midasConnector.setTimeout(0);
    //Broken currently, keep at 0
    //midasConnector.setTimeout(config["timeout-millis"].get<int>());
    //Broken currently
    //midasConnector.SetWatchdogParams(config["call-watchdog"].get<bool>(),static_cast<DWORD>(config["watchdog-timeout-millis"].get<int>()));

    // Call the ConnectToExperiment method
    if (!midasConnector.ConnectToExperiment()) {
        return false;
    }

    // Call the OpenEventBuffer method
    if (!midasConnector.OpenEventBuffer()) {
        return false;
    }

    // Set the buffer cache size if requested
    midasConnector.SetCacheSize(100000);

    // Place a request for a specific event id
    if (!midasConnector.RequestEvent()) {
        return false;
    }

    return true;
}
