#pragma once

#include "back/exec/Record.h"

#include "profiling/ScopeBase.h"

namespace lsql::back::exec {

class Subscriber {
 public:
    virtual ~Subscriber() = default;

    // Feed a record to the subscriber on a given phase.
    // Not threadsafe. Should never be called concurrently.
    //
    // 'record' == nullptr means that the stream has ended. The subscriber
    // must return false. The subscriber will not receive any more records.
    //
    // Record* is transient, meaning it should be clone()-d if intented to
    // be stored beyond the consume() call.
    //
    // Result means "continue sending me records". If false, no more records
    // will be pushed into this subscriber.
    virtual bool consume(int phase, const back::exec::Record* record) = 0;

    virtual prof::ScopeHandleBase scopeHandle() { return prof::ScopeHandleBase(); }
};

}  // namespace lsql::back::exec
