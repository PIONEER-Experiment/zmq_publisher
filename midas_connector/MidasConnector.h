#ifndef MIDAS_CONNECTOR_H
#define MIDAS_CONNECTOR_H

#include "midas.h"
#include "msystem.h"
#include "mrpc.h"
#include "mdsupport.h"
#include "midasio.h"
#include <cstring>

class MidasConnector {
public:
    MidasConnector(const char* clientName);

    // Getters for private variables
    short getEventId() const;
    short getTriggerMask() const;
    int getSamplingType() const; 
    int getBufferSize() const;
    const char* getBufferName() const;
    int getTimeout() const;
    HNDLE getEventBufferHandle() const;

    // Setters for private variables
    void setEventId(short eventId);
    void setTriggerMask(short triggerMask);
    void setSamplingType(int samplingType);
    void setBufferSize(int bufferSize);
    void setBufferName(const char* bufferName);
    void setTimeout(int timeout_millis);
    void setEventBufferHandle(HNDLE eventBufferHandle);

    bool ConnectToExperiment();
    void DisconnectFromExperiment();
    bool OpenEventBuffer();
    bool SetCacheSize(int cacheSize);
    bool RequestEvent();
    bool ReceiveEvent(void* eventBuffer, int& maxEventSize);
    bool SetWatchdogParams(bool callWatchdog, DWORD timeout);


private:
    char client_name_[NAME_LENGTH];
    char host_name_[HOST_NAME_LENGTH];
    char experiment_name_[NAME_LENGTH];
    INT hBufEvent;

    // Private variables with default values
    short event_id;
    short trigger_mask;
    int sampling_type;
    int buffer_size;
    char buffer_name[32];
    int timeout_millis;
};

#endif
