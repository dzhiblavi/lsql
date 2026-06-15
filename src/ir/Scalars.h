#pragma once

#include "ir/Scalar.h"

#include "core/exprs/BinaryExpr.h"
#include "core/exprs/UnaryExpr.h"

#include "core/schema/types.h"
#include "core/types.h"
#include "core/value/Value.h"

namespace lsql::ir {

struct FieldScalar {
    FieldId field_id;
};

struct ValueScalar {
    Value value;
};

struct CoalesceScalar {
    std::vector<Scalar> args;
};

struct CastScalar {
    ValueType cast_to;
    Box<Scalar> expr;
};

struct LikeScalar {
    Box<Scalar> expr;
    std::string regex;
};

struct RSubstrScalar {
    Box<Scalar> expr;
    std::string regex;
};

struct UnaryScalar {
    UnaryExprType type;
    Box<Scalar> expr;
};

struct BinaryScalar {
    BinaryExprType type;
    Box<Scalar> left;
    Box<Scalar> right;
};

struct Scalar {
    ScalarNode node;
    ValueType value_type;
};

}  // namespace lsql::ir
