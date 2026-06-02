#pragma once

#include "back/exec/op/Operation.h"
#include <memory>

namespace lsql::back::exec {

class Source : public virtual Operation {
 public:
    virtual ~Source() = default;

    // Push records for a given phase
    virtual void push(int phase) = 0;
};

using SourcePtr = std::shared_ptr<Source>;

}  // namespace lsql::back::exec
