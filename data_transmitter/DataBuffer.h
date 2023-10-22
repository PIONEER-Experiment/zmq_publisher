#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <cstddef>

template <typename T>
class DataBuffer {
public:
    DataBuffer(size_t size) : bufferSize(size), circularBuffer(size), head(0), tail(0) {}

    void Push(const T& data) {
        circularBuffer[head] = data;
        head = (head + 1) % bufferSize;

        if (head == tail) {
            tail = (tail + 1) % bufferSize; // Remove the oldest event if the buffer is full
        }
    }

    std::vector<T> GetBuffer() const {
        std::vector<T> events;
        
        if (head > tail) {
            events.assign(circularBuffer.begin() + tail, circularBuffer.begin() + head);
        } else {
            events.assign(circularBuffer.begin() + tail, circularBuffer.end());
            events.insert(events.end(), circularBuffer.begin(), circularBuffer.begin() + head);
        }
        
        return events;
    }

    std::string SerializeBuffer() const {
        std::vector<T> buffer = GetBuffer();
        nlohmann::json jsonBuffer(buffer);
        return jsonBuffer.dump();
    }

private:
    std::vector<T> circularBuffer;
    size_t head;
    size_t tail;
    size_t bufferSize;
    
    void Cleanup() {
        // Optionally implement cleanup logic
    }
};

#endif // DATABUFFER_H
