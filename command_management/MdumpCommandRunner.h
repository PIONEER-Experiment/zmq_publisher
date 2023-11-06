#ifndef MDUMPCOMMANDRUNNER_H
#define MDUMPCOMMANDRUNNER_H

#include "CommandRunner.h"

class MdumpCommandRunner : public CommandRunner {
public:
    MdumpCommandRunner(const std::string& mdumpPath);

    void setEventCount(int count);
    void setOutputFormat(const std::string& format);
    void setDisplayMode(const std::string& mode);
    void setBankName(const std::string& bankName);
    void setEventId(int eventId);
    void setTriggerMask(int mask);
    void setSamplingMode(const std::string& mode);
    void setReportBufferRate(bool report);
    void setReportBufferDetails(bool report);
    void setBankFormat(const std::string& format);
    void setDataSource(const std::string& source);
    void enableSerialNumberCheck(bool enable);
    void setBufferName(const std::string& name);
    void setHostname(const std::string& hostname);
    void setExperimentName(const std::string& experiment);
    void setMdumpWaitTime(double seconds);
};

#endif // MDUMPCOMMANDRUNNER_H