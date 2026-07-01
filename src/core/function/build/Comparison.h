#pragma once

#include "core/function/Concepts.h"
#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

template <Comparable T>
struct LessExecutor {
    Value execute(const Value& l, const Value& r) const { return l.get<T>() < r.get<T>(); }
};

template <Comparable T>
struct GreaterExecutor {
    Value execute(const Value& l, const Value& r) const { return l.get<T>() > r.get<T>(); }
};

template <Comparable T>
struct LessEqualExecutor {
    Value execute(const Value& l, const Value& r) const { return l.get<T>() <= r.get<T>(); }
};

template <Comparable T>
struct GreaterEqualExecutor {
    Value execute(const Value& l, const Value& r) const { return l.get<T>() >= r.get<T>(); }
};

static_assert(BinaryExecutor<LessExecutor<int64_t>>);
static_assert(BinaryExecutor<GreaterExecutor<int64_t>>);
static_assert(BinaryExecutor<LessEqualExecutor<int64_t>>);
static_assert(BinaryExecutor<GreaterEqualExecutor<int64_t>>);

template <Comparable T>
inline LessExecutor<T> build(const Less& /*s*/) {
    return {};
}

template <Comparable T>
inline GreaterExecutor<T> build(const Greater& /*s*/) {
    return {};
}

template <Comparable T>
inline LessEqualExecutor<T> build(const LessEqual& /*s*/) {
    return {};
}

template <Comparable T>
inline GreaterEqualExecutor<T> build(const GreaterEqual& /*s*/) {
    return {};
}

}  // namespace lsql::func
