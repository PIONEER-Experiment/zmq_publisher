// HistogramStorage.cpp

#include "HistogramStorage.h"

HistogramStorage::HistogramStorage() {
    // Constructor implementation
}

HistogramStorage::~HistogramStorage() {
    // Destructor implementation
}

HistogramStorage& HistogramStorage::getInstance() {
    static HistogramStorage instance; // Static instance of the class
    return instance;
}

void HistogramStorage::addHistogram(const std::vector<std::string>& histogram) {
    // Add logic to combine histograms or handle them as needed
    // For simplicity, this example just appends the new histogram to the combined one.
    combinedHistogram.insert(combinedHistogram.end(), histogram.begin(), histogram.end());
}

std::vector<std::string> HistogramStorage::getCombinedHistogram() {
    return combinedHistogram;
}
