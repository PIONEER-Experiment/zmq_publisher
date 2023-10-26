#ifndef MIDASBANK_H
#define MIDASBANK_H

#include <string>
#include <vector>

class MidasBank {
public:
    MidasBank(const std::string& bankInfo);
    void displayBankData() const;

    const std::string& getBankName() const;
    int getNumBytes() const;
    const std::vector<int16_t>& getData() const;
    uint64_t* getBankDataAsUint64() const;
    unsigned int getNumUint64Words() const;


private:
    std::string bankName;
    int numBytes;
    std::vector<int16_t> data;
    std::string extractValue(const std::string& input, const std::string& keyword, char endSymbol);
    void parseBankInfo(const std::string& bankInfo);
};

#endif // MIDASBANK_H
