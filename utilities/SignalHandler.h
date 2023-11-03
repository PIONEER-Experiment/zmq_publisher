#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <csignal>
#include <atomic>

class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler();
    
    bool isQuitSignalReceived() const;
    static SignalHandler& getInstance();

private:
    std::atomic<bool> quitSignalReceived;
    void registerSignalHandlers();
    
    static void handleQuitSignal(int signal);
};

#endif // SIGNALHANDLER_H
