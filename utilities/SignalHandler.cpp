#include "SignalHandler.h"

SignalHandler::SignalHandler() {
    // Initialize the flag indicating whether a quit signal is received
    quitSignalReceived.store(false);

    // Register signal handlers in the constructor
    registerSignalHandlers();
}

SignalHandler::~SignalHandler() {
    // Unregister signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}


void SignalHandler::registerSignalHandlers() {
    // Register the signal handler for the interrupt signal (Ctrl+C) and SIGHUP
    signal(SIGINT, handleQuitSignal);
    signal(SIGHUP, handleQuitSignal);
}

bool SignalHandler::isQuitSignalReceived() const {
    return quitSignalReceived.load();
}

void SignalHandler::handleQuitSignal(int signal) {
    if (signal == SIGINT || signal == SIGHUP) {
        getInstance().quitSignalReceived.store(true);
    }
}

SignalHandler& SignalHandler::getInstance() {
    static SignalHandler instance;
    return instance;
}
