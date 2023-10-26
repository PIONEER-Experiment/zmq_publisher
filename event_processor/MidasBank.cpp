#include "MidasBank.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <bitset>


MidasBank::MidasBank(const std::string& bankInfo) {
    parseBankInfo(bankInfo);
}

void MidasBank::parseBankInfo(const std::string& bankInfo) {
    std::istringstream iss(bankInfo);
    bankName = extractValue(bankInfo, "Bank:", ' ');

    std::string line;

    // Skip the lines until we find the line containing "Bank"
    while (std::getline(iss, line)) {
        if (line.find("Bank:") == 0) {
            break; // Found the start of bank data
        }
    }

    // Read and store the data lines into the data vector
    while (std::getline(iss, line)) {
        if (line.empty()) {
            break; // Stop when an empty line is encountered
        }


        // Find the position of "->" in the line
        size_t arrowPos = line.find("->");
        if (arrowPos != std::string::npos) {
            // Extract numbers to the right of "->"
            std::string numbersStr = line.substr(arrowPos + 2); // +2 to skip "->"
            std::istringstream numbersStream(numbersStr);
            int16_t value;
            while (numbersStream >> value) {
                data.push_back(value);
            }
        }
    }

    numBytes = data.size() * sizeof(int);
}

std::string MidasBank::extractValue(const std::string& input, const std::string& keyword, char endSymbol) {
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

void MidasBank::displayBankData() const {
    std::cout << "Bank:" << bankName << " bytes:" << numBytes << std::endl;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 8 == 0) {
            std::cout << std::endl << " " << i + 1 << "->";
        }
        std::cout << " " << data[i];
    }
    std::cout << std::endl;
}

// Define a getter for the bank name
const std::string& MidasBank::getBankName() const {
    return bankName;
}

// Define a getter for the bank length
int MidasBank::getNumBytes() const {
    return numBytes;
}

// Define a getter for the bank data
const std::vector<int16_t>& MidasBank::getData() const {
    return data;
}

uint64_t* MidasBank::getBankDataAsUint64() const {
    const int16_t* int16Data = data.data();
    size_t numInt16Values = data.size();
    
    if (numInt16Values % 4 != 0) {
        // Ensure there's a multiple of 4 values to combine
        throw std::runtime_error("Invalid data size. Must be a multiple of 4.");
    }

    size_t numUint64Values = numInt16Values / 4;
    uint64_t* uint64Data = new uint64_t[numUint64Values];

    for (size_t i = 0; i < numUint64Values; ++i) {
        uint64Data[i] = (static_cast<uint64_t>(data[4 * i + 3] & 0xFFFF) << 48) |
                        (static_cast<uint64_t>(data[4 * i + 2] & 0xFFFF) << 32) |
                        (static_cast<uint64_t>(data[4 * i + 1] & 0xFFFF) << 16) |
                         static_cast<uint64_t>(data[4 * i + 0] & 0xFFFF);
    }

    return uint64Data;
}


unsigned int MidasBank::getNumUint64Words() const {
    // Calculate the number of 64-bit words based on the number of bytes
    // Assuming that each element in data is 4 bytes (32 bits)
    return static_cast<unsigned int>((numBytes / sizeof(uint64_t)) / 2);
}



