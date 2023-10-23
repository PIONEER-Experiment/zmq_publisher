#ifndef EVENT_GRABBER_H
#define EVENT_GRABBER_H

#include "MidasConnector.h"

class EventGrabber {
public:
    EventGrabber(MidasConnector& midasConnector, int maxEventsToGrab = -1, int waitTimeMillis = 0, int verbose = 0);

    bool GrabEvent(void* eventBuffer, int maxEventSize);

    int GetMaxEventsToGrab() const;
    int GetVerbose() const;
    int GetWaitTimeMillis() const;
    int GetEventsGrabbed() const;

    void SetMaxEventsToGrab(int maxEventsToGrab);
    void SetWaitTimeMillis(int waitTimeMillis);
    void SetVerbose(int verbose);
    void SetEventsGrabbed(int eventsGrabbed);

private:
    MidasConnector& midasConnector;
    int maxEventsToGrab;
    int waitTimeMillis;
    int verbose;
    int eventsGrabbed;
};

#endif
