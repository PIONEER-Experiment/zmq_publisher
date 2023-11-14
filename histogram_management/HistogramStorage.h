// HistogramStorage.h

#ifndef HISTOGRAM_STORAGE_H
#define HISTOGRAM_STORAGE_H

#include <vector>
#include <string>
#include <map>
#include "TH1.h"


class HistogramStorage {
public:
    static HistogramStorage& getInstance(); // Singleton instance getter
    
    // Function to get or create a histogram for a given ChannelID
    TH1* getHistogram(std::string key);

    // Function to add data to the histogram for a given ChannelID
    void addToHistogram(std::string key, double data);
    void addToHistogram(std::string key, double dataX, double dataY);

    std::string serialize();

private:
    // Private constructor to enforce singleton pattern
    HistogramStorage();
    ~HistogramStorage();

    // Map to store ChannelID to TH1 histograms
    std::map<std::string, TH1*> histogramMap;
};

#endif // HISTOGRAM_STORAGE_H
