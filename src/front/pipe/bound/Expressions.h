#pragma once

#include "front/ExprKindLevel.h"
#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/fwd/Expr.h"

#include "core/Value.h"
#include "core/exprs/BinaryExpr.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/exprs/UnaryExpr.h"
#include "core/types.h"

#include <string>
#include <vector>

namespace lsql::front::pipe::bound {

struct IdentifierExpr {
    FieldId field_id;
};

struct ValueExpr {
    Value value;
};

struct CastExpr {
    ValueType cast_to;
    Box<Expr> expr;
};

struct InExpr {
    Box<Expr> expr;
    Box<Pipeline> match;
    FieldId match_field_id;
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;
};

struct CoalesceExpr {
    std::vector<Expr> args;
};

struct PercentileExpr {
    Box<Expr> expr;
    std::vector<float> percentiles;
};

struct RSubstrExpr {
    Box<Expr> expr;
    std::string regex;
};

struct BinaryExpr {
    BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

struct CountAllExpr {};

struct UnaryAggregateExpr {
    UnaryAggregateType type;
    Box<Expr> expr;
};

struct UnaryExpr {
    UnaryExprType type;
    Box<Expr> expr;
};

struct Expr {
    ExprNode node;
    ValueType value_type;
    ExprKindLevel level;
    FieldSet required_fields;
};

}  // namespace lsql::front::pipe::bound
