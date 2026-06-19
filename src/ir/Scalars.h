#pragma once

#include "ir/Scalar.h"

#include "core/function/Function.h"

#include "core/schema/types.h"
#include "core/value/Value.h"

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

struct Scalar {
    ScalarNode node;
    ValueType value_type;
};

}  // namespace lsql::ir
