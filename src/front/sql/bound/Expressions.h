#pragma once

#include "front/sql/bound/fwd/Expr.h"
#include "front/sql/bound/fwd/Relation.h"

#include "front/common/bound/ExprKindLevel.h"

#include "core/exprs/BinaryExpr.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/exprs/UnaryExpr.h"

#include "core/schema/FieldSet.h"
#include "core/types.h"
#include "core/value/ValueType.h"

#include <string>
#include <vector>

namespace lsql::front::sql::bound {

struct IdentifierExpr {
    FieldId field_id;
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
    UnaryAggregateType type;
    Box<Expr> expr;
};

struct CountAllExpr {};

struct UnaryExpr {
    UnaryExprType type;
    Box<Expr> expr;
};

struct Expr {
    ExprNode node;
    ValueType value_type;
    common::bound::ExprKindLevel level;
    FieldSet required_fields;
};

}  // namespace lsql::front::sql::bound
