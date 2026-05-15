#pragma once

#include "exec/op/Operation.h"
#include <memory>

namespace lsql::exec {

class Source : public virtual Operation {
 public:
    virtual ~Source() = default;

    // Push records for a given phase
    virtual void push(int phase) = 0;
};

using SourcePtr = std::shared_ptr<Source>;

}  // namespace lsql::exec
