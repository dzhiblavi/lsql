#pragma once

#include "exec/Record.h"

namespace lsql::exec {

class Subscriber {
 public:
    virtual ~Subscriber() = default;

    // record == nullptr means that the stream has ended
    // result means "continue feeding me records"
    // not requried to be threadsafe, thus must not be called concurrently
    virtual bool consume(int phase, const exec::Record* record) = 0;
};

}  // namespace lsql::exec
