#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct SubstrExecutor {
    SubstrExecutor(size_t from, size_t len) : from(from), length(len) {}

    Value execute(const Value& value) const { return value.substr(from, length); }

    size_t from;
    size_t length;
};

static_assert(UnaryExecutor<SubstrExecutor>);

inline SubstrExecutor build(const Substr& s) {
    return {s.from, s.length};
}

}  // namespace lsql::func
