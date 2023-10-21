#include "EventProcessor.h"

EventProcessor::EventProcessor(const std::string& detectorMappingFile, bool verbose)
    : eventUnpacker(new unpackers::BasicEventUnpacker()), // Changed the name to eventUnpacker
      serializer(new unpackers::Serializer(detectorMappingFile,0,0,0)),
      serialized_data(""),
      verbose(verbose) {
}

EventProcessor::~EventProcessor() {
    // Clean up resources
    delete eventUnpacker;
    delete serializer;
}

void EventProcessor::processEvent(void* event_data, INT max_event_size) {
    // Process the event
    TMEvent tmEvent(event_data, max_event_size);

    if (verbose) {
        verbosePrint(tmEvent);
    }

    // Access the WaveformCollection
    eventUnpacker->UnpackEvent(&tmEvent);
    auto waveformCollection = eventUnpacker->GetCollection<dataProducts::Waveform>("WaveformCollection");

    // Serialize the data (you can provide it to the data transmitter)
    serializer->SetEvent(tmEvent.serial_number);
    serializer->SetWaveforms(waveformCollection);
    std::string serializedData = serializer->GetString();
    setSerializedData(serializer->GetString());
}

void EventProcessor::verbosePrint(TMEvent tmEvent) {
    // Access event properties
    printf("Event ID: %d\n", tmEvent.event_id);
    printf("Trigger Mask: %d\n", tmEvent.trigger_mask);
    printf("Serial Number: %d\n", tmEvent.serial_number);
    printf("Time Stamp: %d\n", tmEvent.time_stamp);
    printf("Data Size: %d bytes\n", tmEvent.data_size);
    printf("Event Header Size: %zu bytes\n", tmEvent.event_header_size);
    printf("Bank Header Flags: %u\n", tmEvent.bank_header_flags);

    // Access event banks
    tmEvent.FindAllBanks();
    printf("Number of Banks: %zu\n", tmEvent.banks.size());

    for (const TMBank& bank : tmEvent.banks) {
        printf("Bank Name: %s\n", bank.name.c_str());
        printf("Bank Type: %u\n", bank.type);
        printf("Bank Data Size: %u bytes\n", bank.data_size);
        printf("Bank Data Offset: %zu bytes\n", bank.data_offset);

        // Access bank data
        char* bankData = const_cast<char*>(tmEvent.GetBankData(&bank));

        // Determine the number of entries in the bank
        size_t numEntries = bank.data_size / sizeof(int);

        // Print the number of entries
        printf("Number of Entries: %zu\n", numEntries);

        // Print the first 5 entries
        printf("Data:");
        for (size_t i = 0; i < numEntries; i++) {
            int entryValue = *reinterpret_cast<int*>(bankData + i * sizeof(int));
            printf(" %d", entryValue);
        }
    }
}

// Getter for the serializer
unpackers::Serializer* EventProcessor::getSerializer() {
    return serializer;
}

// Setter for the serializer
void EventProcessor::setSerializer(unpackers::Serializer* newSerializer) {
    serializer = newSerializer;
}

// Getter for the TMEvent
TMEvent EventProcessor::getTMEvent(void* event_data, INT max_event_size) {
    // Create and return a TMEvent from event data
    return TMEvent((int*)event_data, max_event_size);
}

// Getter for the eventUnpacker
unpackers::EventUnpacker* EventProcessor::getEventUnpacker() {
    return eventUnpacker;
}

// Setter for the eventUnpacker
void EventProcessor::setEventUnpacker(unpackers::EventUnpacker* newEventUnpacker) {
    eventUnpacker = newEventUnpacker;
}

// Getter for serialized_data
std::string EventProcessor::getSerializedData() {
    return serialized_data;
}

// Setter for serialized_data
void EventProcessor::setSerializedData(const std::string& data) {
    serialized_data = data;
}