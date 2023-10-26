#include "MdumpCommandManager.h"

MdumpCommandManager::MdumpCommandManager(const std::string& mdumpPath)
    : CommandManager(mdumpPath) { 
}

void MdumpCommandManager::setEventCount(int count) {
    if (count >= 0) {
        commandWithArgs_.push_back("-l");
        commandWithArgs_.push_back(std::to_string(count));
    }
}

void MdumpCommandManager::setOutputFormat(const std::string& format) {
    if (!format.empty()) {
        commandWithArgs_.push_back("-f");
        commandWithArgs_.push_back(format);
    }
}

void MdumpCommandManager::setMdumpWaitTime(double seconds) {
    if (seconds > 0) {
        commandWithArgs_.push_back("-w");
        commandWithArgs_.push_back(std::to_string(seconds));
    }
}

void MdumpCommandManager::setDisplayMode(const std::string& mode) {
    if (!mode.empty()) {
        commandWithArgs_.push_back("-m");
        commandWithArgs_.push_back(mode);
    }
}

void MdumpCommandManager::setBankName(const std::string& bankName) {
    if (!bankName.empty()) {
        commandWithArgs_.push_back("-b");
        commandWithArgs_.push_back(bankName);
    }
}

void MdumpCommandManager::setEventId(int eventId) {
    if (eventId >= 0) {
        commandWithArgs_.push_back("-i");
        commandWithArgs_.push_back(std::to_string(eventId));
    }
}

void MdumpCommandManager::setTriggerMask(int mask) {
    if (mask >= 0) {
        commandWithArgs_.push_back("-k");
        commandWithArgs_.push_back(std::to_string(mask));
    }
}

void MdumpCommandManager::setSamplingMode(const std::string& mode) {
    if (!mode.empty()) {
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back(mode);
    }
}

void MdumpCommandManager::setReportBufferRate(bool report) {
    if (report) {
        commandWithArgs_.push_back("-s");
    } else {
        commandWithArgs_.push_back("-s");
        commandWithArgs_.push_back("-d");
    }
}

void MdumpCommandManager::setBankFormat(const std::string& format) {
    if (!format.empty()) {
        commandWithArgs_.push_back("-t");
        commandWithArgs_.push_back(format);
    }
}

void MdumpCommandManager::setDataSource(const std::string& source) {
    if (!source.empty()) {
        commandWithArgs_.push_back("-x");
        commandWithArgs_.push_back(source);
    }
}

void MdumpCommandManager::enableSerialNumberCheck(bool enable) {
    if (enable) {
        commandWithArgs_.push_back("-y");
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back("all");
    } else {
        commandWithArgs_.push_back("-g");
        commandWithArgs_.push_back("some");
    }
}

void MdumpCommandManager::setBufferName(const std::string& name) {
    if (!name.empty()) {
        commandWithArgs_.push_back("-z");
        commandWithArgs_.push_back(name);
    }
}

void MdumpCommandManager::setHostname(const std::string& hostname) {
    if (!hostname.empty()) {
        commandWithArgs_.push_back("-h");
        commandWithArgs_.push_back(hostname);
    }
}

void MdumpCommandManager::setExperimentName(const std::string& experiment) {
    if (!experiment.empty()) {
        commandWithArgs_.push_back("-e");
        commandWithArgs_.push_back(experiment);
    }
}
