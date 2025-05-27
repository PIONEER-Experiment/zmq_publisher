#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <csignal>
#include <atomic>

/**
 * @brief A class to handle signals, such as interrupt signals (Ctrl+C) and termination signals.
 *
 * The `SignalHandler` class provides functionality to handle signals and determine whether a quit signal is received.
 */
class SignalHandler {
public:
    /**
     * @brief Constructor for SignalHandler.
     */
    SignalHandler();

    /**
     * @brief Destructor for SignalHandler.
     *
     * Unregisters signal handlers during destruction.
     */
    ~SignalHandler();

    /**
     * @brief Checks whether a quit signal is received.
     * @return True if a quit signal is received, false otherwise.
     */
    bool isQuitSignalReceived() const;

    /**
     * @brief Static function to get the singleton instance of SignalHandler.
     * @return Reference to the singleton instance.
     */
    static SignalHandler& getInstance();

private:
    std::atomic<bool> quitSignalReceived;  ///< Atomic flag indicating whether a quit signal is received.

    /**
     * @brief Registers signal handlers during construction.
     */
    void registerSignalHandlers();

    /**
     * @brief Static function to handle quit signals (e.g., SIGINT, SIGHUP, SIGTERM).
     * @param signal The signal number.
     */
    static void handleQuitSignal(int signal);
};

#endif // SIGNALHANDLER_H
