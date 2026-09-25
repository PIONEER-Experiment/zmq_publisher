#pragma once

namespace zmq_publisher {

enum class DeliveryPolicy {
    each,
    batch,
    latest,
    sliding_window,
};

}  // namespace zmq_publisher
