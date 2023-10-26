#include "MidasEvent.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

MidasEvent::MidasEvent(const std::string& eventInfo) {
    parseEventInfo(eventInfo);
}

void MidasEvent::parseEventInfo(const std::string& eventInfo) {
    std::istringstream iss(eventInfo);
    std::string line;
    std::vector<std::string> bankInfoArray;

    eventId = convertToInt(extractValue(eventInfo, "Evid:", '-'));
    mask = convertToInt(extractValue(eventInfo, "Mask:",'-'));
    serialNumber = convertToInt(extractValue(eventInfo, "Serial:",'-'));
    time = convertToUnixTime(extractValue(eventInfo, "Time:",'-'));
    dataSize = convertToInt(extractValue(eventInfo, "Dsize:",'/'));
    numBanks = convertToInt(extractValue(eventInfo, "#banks:",' '));

    // Now, let's find the line with the first mention of "Bank"
    numBanks = 0;
    while (std::getline(iss, line)) {
        if (line.find("Bank:") == 0) {
            // A new bank starts
            std::string bankInfo = line;
            numBanks++;
            // Collect the lines for this bank
            while (std::getline(iss, line)) {
                if (line.find("Bank:") == 0) {
                    // A new bank starts
                    break; // Start processing the next bank
                } else {
                    bankInfo += "\n" + line;
                }
            }
            bankInfoArray.push_back(bankInfo);
        }
    }
    // Initialize MidasBank objects and store them in the banks vector
    for (const std::string& bankInfo : bankInfoArray) {
        MidasBank bank(bankInfo);
        banks.push_back(bank);
    }
}

int MidasEvent::convertToInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return 0; // Handle conversion error, you can modify this as needed
    }
}

unsigned int MidasEvent::convertToUnixTime(const std::string& str) {
    try {
        if (str.substr(0, 2) == "0x") {
            return std::stoul(str, nullptr, 16);
        } else {
            return std::stoi(str);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Conversion error: " << ex.what() << std::endl;
        return 0;
    }
}

std::string MidasEvent::extractValueFromLine(const std::string& line, const std::string& keyword) {
    std::size_t keywordPos = line.find(keyword);
    if (keywordPos != std::string::npos) {
        // Find the start of the value.
        keywordPos += keyword.length();

        // Find the end of the value (up to the next '-' or the end of the line).
        std::size_t endPos = line.find('-', keywordPos);
        if (endPos == std::string::npos) {
            endPos = line.length();
        }

        // Extract the value.
        std::string value = line.substr(keywordPos, endPos - keywordPos);
        return value;
    } else {
        // Keyword not found.
        return "";
    }
}

std::string MidasEvent::extractValue(const std::string& input, const std::string& keyword, char endSymbol) {
    std::string result;
    size_t keywordPos = input.find(keyword);

    if (keywordPos != std::string::npos) {
        keywordPos += keyword.length(); // Move past the keyword
        size_t valueStart = keywordPos;
        size_t valueEnd = input.find(endSymbol, valueStart);

        if (valueEnd != std::string::npos) {
            result = input.substr(valueStart, valueEnd - valueStart);
        } else {
            // If the end symbol is not found, extract the value until the end of the input.
            result = input.substr(valueStart);
        }
    }

    return result;
}

void MidasEvent::displayEventDetails() const {
    std::cout << "------------------------ Event# " << serialNumber << " ------------------------" << std::endl;
    std::cout << "Evid:" << eventId << "- Mask:" << mask << "- Serial:" << serialNumber << "- Time:" << time << "- Dsize:" << dataSize << std::endl;
    std::cout << "#banks:" << numBanks;
    std::cout << std::endl;

    for (size_t i = 0; i < banks.size(); ++i) {
        std::cout << std::endl;
        banks[i].displayBankData();
    }
}

int MidasEvent::getSerialNumber() const {
    return serialNumber;
}

const std::vector<MidasBank>& MidasEvent::getBanks() const {
    return banks;
}