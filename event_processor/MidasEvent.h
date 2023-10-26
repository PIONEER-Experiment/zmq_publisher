#ifndef MIDASEVENT_H
#define MIDASEVENT_H

#include <string>
#include <vector>
#include "MidasBank.h"

class MidasEvent {
public:
    MidasEvent(const std::string& eventInfo);
    void displayEventDetails() const;
    int getSerialNumber() const;
    const std::vector<MidasBank>& getBanks() const;

private:
    int eventId;
    int mask;
    int serialNumber;
    unsigned int time;
    int dataSize;
    int numBanks;
    std::vector<MidasBank> banks;

    void parseEventInfo(const std::string& eventInfo);
    std::string extractValueFromLine(const std::string& line, const std::string& keyword);
    std::string extractValue(const std::string& input, const std::string& keyword, char endSymbol);
    int convertToInt(const std::string& str);
    unsigned int convertToUnixTime(const std::string& str);
};

#endif // MIDASEVENT_H