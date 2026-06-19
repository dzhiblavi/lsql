#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

#include "core/value/cast.h"

namespace lsql::func {

struct CastExecutor {
    explicit CastExecutor(ValueType cast_to) : cast_to(cast_to) {}

    Value execute(Value value) const { return cast(std::move(value), cast_to).value_or(null); }

    ValueType cast_to;
};

static_assert(UnaryExecutor<CastExecutor>);

inline CastExecutor build(const Cast& s) {
    return CastExecutor(s.cast_to);
}

}  // namespace lsql::func
