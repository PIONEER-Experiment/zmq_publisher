// GeneralProcessor.h
#ifndef GENERAL_PROCESSOR_H
#define GENERAL_PROCESSOR_H

#include <string>
#include <vector>

class GeneralProcessor {
public:
    GeneralProcessor(int verbose = 0);
    virtual std::vector<std::string> getProcessedOutput();
    virtual bool isReadyToProcess() const;
    void setVerbose(int verboseLevel);
    virtual int getPeriod() const;
    virtual void setPeriod(int newPeriod); 
    virtual ~GeneralProcessor();

protected:
    int verbose;
    int period;
};

#endif // GENERAL_PROCESSOR_H
