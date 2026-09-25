#pragma once

namespace zmq_publisher {

enum class QueuePolicy { block, drop_newest, drop_oldest, fail };

}  // namespace zmq_publisher
