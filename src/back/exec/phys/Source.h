#pragma once

#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class Source : public virtual Operation {
 public:
    virtual ~Source() = default;
    virtual void push() = 0;
};

}  // namespace lsql::back::exec::phys
