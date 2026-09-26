#pragma once

#include "zmq_publisher/processor/process_result.h"

#include <functional>

namespace zmq_publisher {

class Processor {
public:
    using WakeupHandler = std::function<void()>;

    virtual ~Processor() = default;

    virtual ProcessResult process() = 0;
    virtual bool is_ready() const { return true; }
    virtual bool has_pending_work() const { return false; }
    virtual bool set_wakeup_handler(WakeupHandler) { return false; }
};

}  // namespace zmq_publisher
