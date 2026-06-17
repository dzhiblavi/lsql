#pragma once

#include "back/exec/Record.h"

namespace lsql::back::exec::phys {

class Subscriber {
 public:
    virtual ~Subscriber() = default;

    virtual bool consume(const back::exec::Record* record) = 0;
    virtual prof::ScopeHandleBase scopeHandle() const = 0;
};

}  // namespace lsql::back::exec::phys
