#include "MidasConnector.h"

MidasConnector::MidasConnector(const char* clientName) {
    // Initialize client name
    strncpy(client_name_, clientName, NAME_LENGTH);

    // Get host name and experiment name from environment
    cm_get_environment(host_name_, sizeof(host_name_), experiment_name_, sizeof(experiment_name_));

    // Initialize other private variables if needed
    event_id = EVENTID_ALL;  // Initialize with default value
    trigger_mask = TRIGGER_ALL;  // Initialize with default value
    sampling_type = GET_ALL;  // Initialize with default value (renamed from get_flags)
    buffer_size = DEFAULT_BUFFER_SIZE;  // Initialize with default value
    timeout_millis = BM_WAIT;
    strncpy(buffer_name, EVENT_BUFFER_NAME, sizeof(buffer_name));  // Initialize with default value
}

// Getters for the private variables
short MidasConnector::getEventId() const {
    return event_id;
}

short MidasConnector::getTriggerMask() const {
    return trigger_mask;
}

int MidasConnector::getSamplingType() const {
    return sampling_type;  
}

int MidasConnector::getBufferSize() const {
    return buffer_size;
}

const char* MidasConnector::getBufferName() const {
    return buffer_name;
}

int MidasConnector::getTimeout() const {
    return timeout_millis;
}

HNDLE MidasConnector::getEventBufferHandle() const {
    return hBufEvent;
}

// Setters for the private variables
void MidasConnector::setEventId(short eventId) {
    event_id = eventId;
}

void MidasConnector::setTriggerMask(short triggerMask) {
    trigger_mask = triggerMask;
}

void MidasConnector::setSamplingType(int samplingType) {
    sampling_type = samplingType;  
}

void MidasConnector::setBufferSize(int bufferSize) {
    buffer_size = bufferSize;
}

void MidasConnector::setBufferName(const char* bufferName) {
    strncpy(buffer_name, bufferName, sizeof(buffer_name));
}

void MidasConnector::setTimeout(int timeoutMillis) {
    timeout_millis = timeoutMillis;
}

void MidasConnector::setEventBufferHandle(HNDLE eventBufferHandle) {
    hBufEvent = eventBufferHandle;
}


bool MidasConnector::ConnectToExperiment() {
    // Connect to the experiment
    int status = cm_connect_experiment(host_name_, experiment_name_, client_name_, NULL);
    if (status != CM_SUCCESS) {
        // Handle connection error
        return false;
    }
    return true;
}

void MidasConnector::DisconnectFromExperiment() {
    // Disconnect from the experiment
    cm_disconnect_experiment();
}

bool MidasConnector::OpenEventBuffer() {
    int status = bm_open_buffer(buffer_name, buffer_size, &hBufEvent);
    if (status != BM_SUCCESS && status != BM_CREATED) {
        cm_msg(MERROR, client_name_, "Cannot open buffer \"%s\", bm_open_buffer() status %d", buffer_name, status);
        return false;
    }
    return true;
}

bool MidasConnector::SetCacheSize(int cacheSize) {
    bm_set_cache_size(hBufEvent, cacheSize, 0);
    return true;
}

bool MidasConnector::RequestEvent() {
    int request_id;
    int status = bm_request_event(hBufEvent, event_id, trigger_mask, sampling_type, &request_id, NULL);
    return status == BM_SUCCESS;
}

bool MidasConnector::ReceiveEvent(void* eventBuffer, int& maxEventSize) {
    int status = bm_receive_event(hBufEvent, eventBuffer, &maxEventSize, timeout_millis);
    return status == BM_SUCCESS;
}

