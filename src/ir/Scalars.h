#pragma once

#include "ir/Scalar.h"

#include "core/function/Function.h"

#include "core/schema/types.h"
#include "core/types.h"
#include "core/value/Value.h"

#include <cstddef>
#include <functional>

namespace lsql::ir {

struct FieldScalar {
    FieldId field_id;
};

struct ValueScalar {
    Value value;
};

// Coalesce has weird argument calculation policy
// that does not match generic functions.
struct CoalesceScalar {
    std::vector<Scalar> args;
};

struct FnCallScalar {
    func::Function function;
    std::vector<Scalar> args;
};

struct CachedScalar {
    Box<Scalar> scalar;
};

struct Scalar {
    ScalarNode node;
    ValueType value_type;
};

bool operator==(const Scalar& left, const Scalar& right);

}  // namespace lsql::ir

template <>
struct std::hash<lsql::ir::Scalar> {
    size_t operator()(const lsql::ir::Scalar& scalar) const;
};
