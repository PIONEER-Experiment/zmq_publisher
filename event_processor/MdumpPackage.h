#ifndef MDUMP_PACKAGE_H
#define MDUMP_PACKAGE_H

#include <vector>
#include "MidasEvent.h" // Include the MidasEvent class header

class MdumpPackage {
public:
    MdumpPackage(const std::string& mdumpOutput);
    void displayEventsDetails() const;

    // Add any other methods or members you need
private:
    std::vector<MidasEvent> events;

    // Private method to parse the mdump output
    void parseMdumpInfo(const std::string& mdumpOutput);
};

#endif
