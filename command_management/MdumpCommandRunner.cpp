#include "MdumpCommandRunner.h"

MdumpCommandRunner::MdumpCommandRunner(const std::string& mdumpPath)
    : CommandRunner(mdumpPath) { 
}

void MdumpCommandRunner::setEventCount(int count) {
    if (count >= 0) {
        commandWithArgs_.push_back("-l");
        commandWithArgs_.push_back(std::to_string(count));
    }
}

void MdumpCommandRunner::setOutputFormat(const std::string& format) {
    if (!format.empty()) {
        commandWithArgs_.push_back("-f");
        commandWithArgs_.push_back(format);
    }
}

void MdumpCommandRunner::setMdumpWaitTime(double seconds) {
    if (seconds > 0) {
        commandWithArgs_.push_back("-w");
        commandWithArgs_.push_back(std::to_string(seconds));
    }
}

void MdumpCommandRunner::setDisplayMode(const std::string& mode) {
    if (!mode.empty()) {
        commandWithArgs_.push_back("-m");
        commandWithArgs_.push_back(mode);
    }
}

void MdumpCommandRunner::setBankName(const std::string& bankName) {
    if (!bankName.empty()) {
        commandWithArgs_.push_back("-b");
        commandWithArgs_.push_back(bankName);
    }
}

void MdumpCommandRunner::setEventId(int eventId) {
    if (eventId >= 0) {
        commandWithArgs_.push_back("-i");
        commandWithArgs_.push_back(std::to_string(eventId));
    }
}

void MdumpCommandRunner::setTriggerMask(int mask) {
    if (mask >= 0) {
        commandWithArgs_.push_back("-k");
        commandWithArgs_.push_back(std::to_string(mask));
    }
}

void MdumpCommandRunner::setSamplingMode(const std::string& mode) {
    if (!mode.empty()) {
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back(mode);
    }
}

void MdumpCommandRunner::setReportBufferRate(bool report) {
    if (report) {
        commandWithArgs_.push_back("-s");
    } else {
        commandWithArgs_.push_back("-s");
        commandWithArgs_.push_back("-d");
    }
}

void MdumpCommandRunner::setBankFormat(const std::string& format) {
    if (!format.empty()) {
        commandWithArgs_.push_back("-t");
        commandWithArgs_.push_back(format);
    }
}

void MdumpCommandRunner::setDataSource(const std::string& source) {
    if (!source.empty()) {
        commandWithArgs_.push_back("-x");
        commandWithArgs_.push_back(source);
    }
}

void MdumpCommandRunner::enableSerialNumberCheck(bool enable) {
    if (enable) {
        commandWithArgs_.push_back("-y");
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back("all");
    } else {
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back("some");
    }
}

void MdumpCommandRunner::setBufferName(const std::string& name) {
    if (!name.empty()) {
        commandWithArgs_.push_back("-z");
        commandWithArgs_.push_back(name);
    }
}

void MdumpCommandRunner::setHostname(const std::string& hostname) {
    if (!hostname.empty()) {
        commandWithArgs_.push_back("-h");
        commandWithArgs_.push_back(hostname);
    }
}

void MdumpCommandRunner::setExperimentName(const std::string& experiment) {
    if (!experiment.empty()) {
        commandWithArgs_.push_back("-e");
        commandWithArgs_.push_back(experiment);
    }
}
