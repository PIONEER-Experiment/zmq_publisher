#ifndef ODBPROCESSOR_H
#define ODBPROCESSOR_H

#include "CommandProcessor.h"

class ODBProcessor : public CommandProcessor {
public:
    ODBProcessor(int verbose);

    std::vector<std::string> ProcessCommandOutput(const std::string& commandOutput) override;
};

#endif
