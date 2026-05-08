#pragma once

#include "exec/op/Operation.h"

namespace lsql::exec {

class Source : public Operation {
 public:
    using Operation::Operation;
    virtual void push(int phase) = 0;
};

using SourcePtr = std::shared_ptr<Source>;

}  // namespace lsql::exec
