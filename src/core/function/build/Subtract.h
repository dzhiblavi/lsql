#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

template <Subtractable T>
struct SubtractExecutor {
    Value execute(const Value& l, const Value& r) const { return l.get<T>() - r.get<T>(); }
};

static_assert(BinaryExecutor<SubtractExecutor<int64_t>>);

template <Subtractable T>
inline SubtractExecutor<T> build(const Subtract& /*s*/) {
    return {};
}

}  // namespace lsql::func
