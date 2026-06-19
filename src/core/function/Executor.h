#pragma once

#include "core/value/Value.h"

#include <span>

namespace lsql::func {

class Executor {
 public:
    virtual ~Executor() = default;

    virtual Value execute(std::span<Value> values) const = 0;
};

}  // namespace lsql::func
