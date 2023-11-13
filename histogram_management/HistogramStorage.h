// HistogramStorage.h

#ifndef HISTOGRAM_STORAGE_H
#define HISTOGRAM_STORAGE_H

#include <vector>
#include <string>

class HistogramStorage {
public:
    static HistogramStorage& getInstance(); // Singleton instance getter
    void addHistogram(const std::vector<std::string>& histogram);
    std::vector<std::string> getCombinedHistogram();

private:
    // Private constructor to enforce singleton pattern
    HistogramStorage();
    ~HistogramStorage();

    // Member to store the histograms
    std::vector<std::string> combinedHistogram;
};

#endif // HISTOGRAM_STORAGE_H
