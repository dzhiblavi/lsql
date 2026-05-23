#pragma once

#include "iface/sql/bind/Expr.h"
#include "iface/sql/bind/Relation.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/ValueType.h"
#include "core/expressions.h"
#include "core/types.h"

#include <string>
#include <vector>

namespace lsql::iface::sql::bind {

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
    Box<Relation> match;
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

struct UnaryAggregateExpr {
    UnaryAggregateExprType type;
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

}  // namespace lsql::iface::sql::bind
