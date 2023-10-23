#include "EventGrabber.h"
#include "ProjectPrinter.h"
#include <chrono>
#include <thread>

EventGrabber::EventGrabber(MidasConnector& midasConnector, int maxEventsToGrab, int waitTimeMillis, int verbose)
    : midasConnector(midasConnector), maxEventsToGrab(maxEventsToGrab), waitTimeMillis(waitTimeMillis), verbose(verbose), eventsGrabbed(0) {}

bool EventGrabber::GrabEvent(void* eventBuffer, int maxEventSize) {
    if (maxEventsToGrab < 0 || eventsGrabbed % maxEventsToGrab != 0) {
        bool success = (midasConnector.ReceiveEvent(eventBuffer, maxEventSize) == BM_SUCCESS);
        if (success) {
            eventsGrabbed += 1;
        }
        return success;
    }

    if (verbose > 0) {
        ProjectPrinter printer;
        printer.Print("Waiting for " + std::to_string(waitTimeMillis) + " milliseconds before the next event grab.");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMillis));
    return true;  // You can return an appropriate value based on your needs
}

int EventGrabber::GetMaxEventsToGrab() const {
    return maxEventsToGrab;
}

void EventGrabber::SetMaxEventsToGrab(int maxEventsToGrab) {
    this->maxEventsToGrab = maxEventsToGrab;
}

int EventGrabber::GetWaitTimeMillis() const {
    return waitTimeMillis;
}

void EventGrabber::SetWaitTimeMillis(int waitTimeMillis) {
    this->waitTimeMillis = waitTimeMillis;
}

int EventGrabber::GetVerbose() const {
    return verbose;
}

void EventGrabber::SetVerbose(int verbose) {
    this->verbose = verbose;
}

int EventGrabber::GetEventsGrabbed() const {
    return eventsGrabbed;
}

void EventGrabber::SetEventsGrabbed(int eventsGrabbed) {
    this->eventsGrabbed = eventsGrabbed;
}
