#include "MidasBank.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <bitset>
#include <algorithm> // For std::find

/*
I don't like that I had to hardcode these two data types in,
but I really didn't see another way.
A template class wouldn't work as MidasEvent stores a vector of MidasBanks.
I.e. it couldn't support different template classes in the vector.
Perhaps a better way is to use polymorphism, i.e. have different bank data types
be different derived classes. But this has drawbacks:
    1. Is equal (or more) code to write
    2. Data getters would have to be serialized to a common type (probably string)
    3. Arguably doesn't lay down any additional (useful) framework for furture use cases
*/

// List of allowed data types
const std::vector<std::string> allowedDataTypes = { "INT16", "INT32" };


MidasBank::MidasBank(const std::string& bankInfo, const std::string& dataType) {
    // Check if the given data type is in the list of allowed data types
    if (std::find(allowedDataTypes.begin(), allowedDataTypes.end(), dataType) != allowedDataTypes.end()) {
        // If the data type is allowed, set it
        this->dataType = dataType;
    } else {
        // If not in the allowed list, default to "Int32"
        this->dataType = "INT32";
    }

    // Now parse the bank info
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

    // Read and store the data lines into the appropriate data vector based on the data type
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
            if (dataType == "INT16") {
                int16_t value;
                while (numbersStream >> value) {
                    int16Data.push_back(value);
                }
            } else { // Default to 32-bit integers
                int32_t value;
                while (numbersStream >> value) {
                    int32Data.push_back(value);
                }
            }
        }
    }

    // Set the number of bytes based on the size of the data vectors
    if (dataType == "INT16") {
        numBytes = int16Data.size() * sizeof(int16_t);
    } else {
        numBytes = int32Data.size() * sizeof(int32_t);
    }
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
    
    // Determine which data vector to use based on the specified dataType
    const std::vector<int16_t>& data16 = (dataType == "INT16") ? int16Data : std::vector<int16_t>();
    const std::vector<int32_t>& data32 = (dataType == "INT32") ? int32Data : std::vector<int32_t>();

    // Display data based on the selected dataType
    if (dataType == "INT16") {
        for (size_t i = 0; i < data16.size(); ++i) {
            if (i % 8 == 0) {
                std::cout << std::endl << " " << i + 1 << "->";
            }
            std::cout << " " << data16[i];
        }
    } else if (dataType == "INT32") {
        for (size_t i = 0; i < data32.size(); ++i) {
            if (i % 8 == 0) {
                std::cout << std::endl << " " << i + 1 << "->";
            }
            std::cout << " " << data32[i];
        }
    } else {
        std::cout << "Invalid data type specified: " << dataType << std::endl;
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

// Define a getter for the 16 bit int bank data
const std::vector<int16_t>& MidasBank::getInt16Data() const {
    return int16Data;
}

// Define a getter for the 32 bit int bank data
const std::vector<int32_t>& MidasBank::getInt32Data() const {
    return int32Data;
}


uint64_t* MidasBank::getBankDataAsUint64() const {
    size_t numValues;
    const std::vector<int16_t>* dataPtr16 = nullptr;
    const std::vector<int32_t>* dataPtr32 = nullptr;

    if (dataType == "INT16") {
        numValues = int16Data.size();
        dataPtr16 = &int16Data;
    } else if (dataType == "INT32") {
        numValues = int32Data.size();
        dataPtr32 = &int32Data;
    } else {
        throw std::runtime_error("Invalid data type specified: " + dataType);
    }

    if ((dataType == "INT16" && numValues % 4 != 0) || (dataType == "INT32" && numValues % 2 != 0)) {
        // Ensure there's a valid number of values
        throw std::runtime_error("Invalid data size.");
    }

    size_t numUint64Values = (dataType == "int16") ? numValues / 4 : numValues / 2;
    uint64_t* uint64Data = new uint64_t[numUint64Values];

    for (size_t i = 0; i < numUint64Values; ++i) {
        if (dataPtr16) {
            uint64Data[i] = (static_cast<uint64_t>((*dataPtr16)[4 * i + 3] & 0xFFFF) << 48) |
                            (static_cast<uint64_t>((*dataPtr16)[4 * i + 2] & 0xFFFF) << 32) |
                            (static_cast<uint64_t>((*dataPtr16)[4 * i + 1] & 0xFFFF) << 16) |
                            static_cast<uint64_t>((*dataPtr16)[4 * i + 0] & 0xFFFF);
        } else if (dataPtr32) {
            uint64Data[i] = (static_cast<uint64_t>((*dataPtr32)[2 * i + 1] & 0xFFFFFFFF) << 32) |
                            static_cast<uint64_t>((*dataPtr32)[2 * i + 0] & 0xFFFFFFFF);
        }
    }

    return uint64Data;
}



unsigned int MidasBank::getNumUint64Words() const {
    // Calculate the number of 64-bit words based on the number of bytes
    // Assuming that each element in data is 4 bytes (32 bits)
    return static_cast<unsigned int>((numBytes / sizeof(uint64_t)) / 2);
}



