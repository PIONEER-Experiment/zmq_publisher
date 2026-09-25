#pragma once

#include "zmq_publisher/processor/process_result.h"

namespace zmq_publisher {

class Processor {
public:
    virtual ~Processor() = default;

    virtual ProcessResult process() = 0;
    virtual bool is_ready() const { return true; }
};

}  // namespace zmq_publisher
