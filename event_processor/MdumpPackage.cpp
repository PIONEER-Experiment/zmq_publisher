#include "MdumpPackage.h"
#include <iostream>

MdumpPackage::MdumpPackage(const std::string& mdumpOutput) {
    parseMdumpInfo(mdumpOutput);
}

void MdumpPackage::parseMdumpInfo(const std::string& mdumpOutput) {
    std::vector<std::string> eventStrings;

    // Split the input string into an array of event strings
    std::size_t pos = mdumpOutput.find("------------------------ Event# ");
    while (pos != std::string::npos) {
        std::size_t nextPos = mdumpOutput.find("------------------------ Event# ", pos + 1);
        std::string eventStr = mdumpOutput.substr(pos, nextPos - pos);
        eventStrings.push_back(eventStr);

        pos = nextPos;
    }

    // Loop over event strings and construct MidasEvent objects
    for (const std::string& eventStr : eventStrings) {
        MidasEvent event(eventStr);
        events.push_back(event);
    }
}

void MdumpPackage::displayEventsDetails() const {
    std::cout << "All Events:\n";
    for (const MidasEvent& event : events) {
        event.displayEventDetails();
    }
}

const std::vector<MidasEvent>& MdumpPackage::getEvents() const {
    return events;
}