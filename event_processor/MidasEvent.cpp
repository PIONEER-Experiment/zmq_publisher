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
    std::vector<std::string> bankTypes;

    eventId = convertToInt(extractValue(eventInfo, "Evid:", '-'));
    mask = convertToInt(extractValue(eventInfo, "Mask:",'-'));
    serialNumber = convertToInt(extractValue(eventInfo, "Serial:",'-'));
    time = convertToUnixTime(extractValue(eventInfo, "Time:",'-'));
    dataSize = convertToInt(extractValue(eventInfo, "Dsize:",'/'));
    numBanks = convertToInt(extractValue(eventInfo, "#banks:",' '));

    // Now, let's find the line with the first mention of "Bank"
    numBanks = 0;
    std::string bankInfo; // Variable to store information about the current bank
    bool foundFirstBank = false; // Flag to indicate whether the first bank is found
    while (std::getline(iss, line)) {
        if (line.find("Bank:") == 0) {
            foundFirstBank = true; // Set flag indicating the first bank is found
            // A new bank starts
            if (!bankInfo.empty()) {
                // If bankInfo is not empty, store the information of the previous bank
                bankInfoArray.push_back(bankInfo);
                bankInfo.clear(); // Clear the bankInfo variable for the new bank
            }
            // Start collecting lines for the new bank
            bankInfo += line;
            
            // Now let's check for the bank type
            size_t typePos = line.find("Type:");
            if (typePos != std::string::npos) {
                std::string type = line.substr(typePos + 5); // Skip "Type:" and space
                if (type.find("Signed Integer*2") != std::string::npos) {
                    bankTypes.push_back("INT16");
                } else if (type.find("Unsigned Integer*4") != std::string::npos) {
                    bankTypes.push_back("INT32");
                } else {
                    // Handle unrecognized bank types
                    bankTypes.push_back("UNKNOWN");
                }
            } else {
                // Handle cases where bank type is not specified
                bankTypes.push_back("UNKNOWN");
            }
        } else if (foundFirstBank) { // Only collect lines for banks after finding the first bank
            // Continue collecting lines for the current bank
            bankInfo += "\n" + line;
        }
    }
    // Store the information of the last bank (if any)
    if (!bankInfo.empty()) {
        bankInfoArray.push_back(bankInfo);
    }
    // Initialize MidasBank objects and store them in the banks vector
    for (size_t i = 0; i < bankInfoArray.size(); ++i) {
        MidasBank bank(bankInfoArray[i], bankTypes[i]);
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