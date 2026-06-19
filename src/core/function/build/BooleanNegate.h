#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct BooleanNegateExecutor {
    Value execute(const Value& value) const { return !value.get<bool>(); }
};

static_assert(UnaryExecutor<BooleanNegateExecutor>);

inline BooleanNegateExecutor build(const BooleanNegate& /*s*/) {
    return {};
}

}  // namespace lsql::func
