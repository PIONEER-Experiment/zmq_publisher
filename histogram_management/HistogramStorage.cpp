#include "HistogramStorage.h"
#include "ProjectPrinter.h"
#include "TBufferJSON.h"
#include "TString.h"
#include "TH1D.h"
#include "TH2D.h"
#include "nlohmann/json.hpp"


HistogramStorage::HistogramStorage() {
    // Constructor implementation
}

HistogramStorage::~HistogramStorage() {
    // Destructor implementation
    for (auto& entry : histogramMap) {
        delete entry.second; // Release memory for each histogram
    }
}

HistogramStorage& HistogramStorage::getInstance() {
    static HistogramStorage instance;
    return instance;
}

TH1* HistogramStorage::getHistogram(std::string key) {
    // Implementation to get or create a histogram
    auto it = histogramMap.find(key);
    if (it != histogramMap.end()) {
        // key exists, return a pointer to the existing histogram
        return it->second;
    } else {
        return nullptr;
    }
}

void HistogramStorage::addToHistogram(std::string key, double data) {
    // Check if the key exists in the map
    auto it = histogramMap.find(key);

    if (it != histogramMap.end()) {
        // key exists, add data to the existing histogram
        it->second->Fill(data);
    } else {
        // key doesn't exist, create a new histogram and add data
        //TH1D(name, title, bins, min, max)
        TH1D* newHistogram = new TH1D(("Histogram_" + key).c_str(), "", 100, 0, 250000);
        newHistogram->Fill(data);

        // Add the new histogram to the map
        histogramMap[key] = newHistogram;
    }

    //ProjectPrinter printer;
    //TString histoTstring = TBufferJSON::ToJSON(histogramMap[key]);
    //std::string histoString(histoTstring.Data());
    //printer.Print(histoString);
}

void HistogramStorage::addToHistogram(std::string key, double dataX, double dataY) {
    // Check if the key exists in the map
    auto it = histogramMap.find(key);

    if (it != histogramMap.end()) {
        // key exists, add data to the existing histogram
        it->second->Fill(dataX,dataY);
    } else {
        // key doesn't exist, create a new histogram and add data
        //TH2D(name, title, Xbins, Xmin, Xmax, Ybins, Ymin, Ymax)
        TH2D* newHistogram = new TH2D(("Histogram_" + key).c_str(), "", 12, 0, 11, 12, 0 , 11);
        newHistogram->Fill(dataX,dataY);

        // Add the new histogram to the map
        histogramMap[key] = newHistogram;
    }

    //ProjectPrinter printer;
    //TString histoTstring = TBufferJSON::ToJSON(histogramMap[key]);
    //std::string histoString(histoTstring.Data());
    //printer.Print(histoString);
}

std::string HistogramStorage::serialize() {
    nlohmann::json serializedHistogramMap;

    for (const auto& entry : histogramMap) {
        const std::string& key = entry.first;
        const TH1* histogram = entry.second;

        // Serialize the histogram to a string (you might want to adjust this part based on your needs)
        TString histoTstring = TBufferJSON::ToJSON(histogram);
        std::string histoString(histoTstring.Data());

        try {
            nlohmann::json histoJson = nlohmann::json::parse(histoString);
            serializedHistogramMap[key] = histoJson;
        } catch (const nlohmann::json::exception& e) {
            ProjectPrinter printer;
            printer.PrintWarning("Error parsing histogram JSON for key " + key + ": " + std::string(e.what()), __LINE__, __FILE__);
        }
    }

    // Return the JSON string
    return serializedHistogramMap.dump();
}

