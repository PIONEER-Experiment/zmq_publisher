#ifndef GENERAL_PROCESSOR_H
#define GENERAL_PROCESSOR_H

#include <string>
#include <vector>

class GeneralProcessor {
public:
    GeneralProcessor(int verbose = 0);  // Add a constructor with verbose parameter
    std::vector<std::string> processOutput(const std::string& output);

protected:
    int verbose;  // Make verbose accessible to derived classes
};

#endif
