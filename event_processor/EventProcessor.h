#ifndef EVENTPROCESSOR_H
#define EVENTPROCESSOR_H

#include "midas.h"
#include "midasio.h"
#include "unpackers/BasicEventUnpacker.hh"
#include "unpackers/EventUnpacker.hh" // Include the base EventUnpacker class
#include "serializer/Serializer.hh"
#include "dataProducts/Waveform.hh"
#include "dataProducts/Performance.hh"
#include <string>
#include <vector>
#include "MidasEvent.h"

class EventProcessor {
public:
    EventProcessor(const std::string& detectorMappingFile, int verbose = 0);
    ~EventProcessor();

    int processEvent(void* event_data, INT max_event_size);
    //int processEvent(const MidasEvent& event, const std::string& bankName);
    int processWaveformDataBank(const MidasEvent& event, const std::string& bankPrefix);
    int processPerformanceDataBank(const MidasEvent& event, const std::string& bankPrefix);

    // Getter for the serializer
    unpackers::Serializer* getSerializer();

    // Setter for the serializer
    void setSerializer(unpackers::Serializer* newSerializer);

    // Getter for the eventUnpacker
    unpackers::EventUnpacker* getEventUnpacker();

    // Setter for the eventUnpacker
    void setEventUnpacker(unpackers::EventUnpacker* newUnpacker);

    // Getter for serialized_data
    std::vector<std::string> getSerializedData();

    // Setter for serialized_data
    void setSerializedData(const std::vector<std::string>& data);

    // Getter for the TMEvent
    TMEvent getTMEvent(void* event_data, INT max_event_size);

    // Getter for waveformCollection
    std::vector<dataProducts::Waveform> getWaveforms();

private:
    unpackers::EventUnpacker* eventUnpacker;
    unpackers::Serializer* serializer;
    std::vector<std::string> serialized_data;
    int verbose;
    int lastSerialNumberProcessed;
    std::vector<dataProducts::Waveform> waveforms;

    void verbosePrint(TMEvent tmEvent);
    void updateLastSerialNumberProcessed(int serialNumber);
    bool isNewEvent(MidasEvent event);
};

#endif // EVENTPROCESSOR_H
