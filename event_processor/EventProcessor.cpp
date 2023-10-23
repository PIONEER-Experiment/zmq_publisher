#include "EventProcessor.h"
#include "ProjectPrinter.h"

EventProcessor::EventProcessor(const std::string& detectorMappingFile, int verbose)
    : eventUnpacker(new unpackers::BasicEventUnpacker()), // Changed the name to eventUnpacker
      serializer(new unpackers::Serializer(detectorMappingFile, 0, 0, 0)),
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

    if (verbose > 2) {
        ProjectPrinter printer;
        printer.Print(std::to_string(verbose));
        verbosePrint(tmEvent);
    }

    // Access the WaveformCollection
    eventUnpacker->UnpackEvent(&tmEvent);
    auto waveformCollection = eventUnpacker->GetCollection<dataProducts::Waveform>("WaveformCollection");

    // Serialize the data (you can provide it to the data transmitter)
    serializer->SetEvent(tmEvent.serial_number);
    serializer->SetWaveforms(waveformCollection);
    std::string serializedData = serializer->GetString();
    setSerializedData(serializedData);
}

void EventProcessor::verbosePrint(TMEvent tmEvent) {
    // Use ProjectPrinter for verbose output
    ProjectPrinter printer;

    // Access event properties
    printer.Print("Event ID: " + std::to_string(tmEvent.event_id));
    printer.Print("Trigger Mask: " + std::to_string(tmEvent.trigger_mask));
    printer.Print("Serial Number: " + std::to_string(tmEvent.serial_number));
    printer.Print("Time Stamp: " + std::to_string(tmEvent.time_stamp));
    printer.Print("Data Size: " + std::to_string(tmEvent.data_size) + " bytes");
    printer.Print("Event Header Size: " + std::to_string(tmEvent.event_header_size) + " bytes");
    printer.Print("Bank Header Flags: " + std::to_string(tmEvent.bank_header_flags));

    // Access event banks
    tmEvent.FindAllBanks();
    printer.Print("Number of Banks: " + std::to_string(tmEvent.banks.size()));

    for (const TMBank& bank : tmEvent.banks) {
        printer.Print("Bank Name: " + bank.name);
        printer.Print("Bank Type: " + std::to_string(bank.type));
        printer.Print("Bank Data Size: " + std::to_string(bank.data_size) + " bytes");
        printer.Print("Bank Data Offset: " + std::to_string(bank.data_offset) + " bytes");

        // Access bank data
        char* bankData = const_cast<char*>(tmEvent.GetBankData(&bank));

        // Determine the number of entries in the bank
        size_t numEntries = bank.data_size / sizeof(int);

        // Print the number of entries
        printer.Print("Number of Entries: " + std::to_string(numEntries));

        // Print the first 5 entries
        printer.Print("Data: ");
        std::string dataString;

        for (size_t i = 0; i < numEntries; i++) {
            int entryValue = *reinterpret_cast<int*>(bankData + i * sizeof(int));
            dataString += std::to_string(entryValue) + " ";
        }

        printer.Print(dataString);
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