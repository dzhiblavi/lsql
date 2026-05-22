#pragma once

#include "ir/Expr.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/expressions.h"
#include "core/types.h"

namespace lsql::ir {

struct FieldExpr {
    FieldId field_id;
};

struct ValueExpr {
    Value value;
};

struct CoalesceExpr {
    std::vector<Expr> args;
};

struct CastExpr {
    ValueType cast_to;
    Box<Expr> expr;
};

struct PercentileExpr {
    Box<Expr> expr;
    std::vector<float> percentiles;
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;
};

struct RSubstrExpr {
    Box<Expr> expr;
    std::string regex;
};

struct UnaryExpr {
    UnaryExprType type;
    Box<Expr> expr;
};

struct UnaryAggregateExpr {
    UnaryAggregateExprType type;
    Box<Expr> expr;
};

struct BinaryExpr {
    BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

struct Expr {
    ExprNode node;
    ValueType value_type;
    ExprKindLevel level;
};

}  // namespace lsql::ir
