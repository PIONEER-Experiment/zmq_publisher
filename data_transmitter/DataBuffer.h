// DATABUFFER_H
#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <cstddef>

/**
 * @brief A circular buffer for storing data of a specified type.
 * 
 * The `DataBuffer` class implements a circular buffer to store data of a specified type.
 * It allows pushing new data into the buffer and provides methods to retrieve and serialize
 * the buffered data.
 * 
 * @tparam T The type of data to be stored in the buffer.
 */
template <typename T>
class DataBuffer {
public:
    /**
     * @brief Constructor for DataBuffer with a specified size.
     * @param size The size of the circular buffer.
     */
    DataBuffer(size_t size) : bufferSize(size), circularBuffer(size), head(0), tail(0) {}

    /**
     * @brief Pushes new data into the circular buffer.
     * @param data The data to be pushed into the buffer.
     */
    void Push(const T& data) {
        circularBuffer[head] = data;
        head = (head + 1) % bufferSize;

        if (head == tail) {
            tail = (tail + 1) % bufferSize; // Remove the oldest event if the buffer is full
        }
    }

    /**
     * @brief Gets the buffer content as a vector.
     * @return A vector containing the buffered data.
     */
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

    /**
     * @brief Serializes the buffer content to a JSON string.
     * @return A JSON string representing the buffered data.
     */
    std::string SerializeBuffer() const {
        std::vector<T> buffer = GetBuffer();
        nlohmann::json jsonBuffer(buffer);
        return jsonBuffer.dump();
    }

private:
    std::vector<T> circularBuffer; ///< The circular buffer storing the data.
    size_t head; ///< The index of the head in the circular buffer.
    size_t tail; ///< The index of the tail in the circular buffer.
    size_t bufferSize; ///< The size of the circular buffer.
    
    /**
     * @brief Optional method for cleanup logic.
     */
    void Cleanup() {
        // Optionally implement cleanup logic
    }
};

#endif // DATABUFFER_H
