#pragma once

#include "iface/sql/bind/Expr.h"
#include "iface/sql/bind/ExprKindLevel.h"
#include "iface/sql/bind/Relation.h"

#include "core/Value.h"
#include "core/types.h"

namespace lsql::iface::sql::bind {

ValueType valueTypeOf(const Expr& e);
ExprKindLevel exprKindLevelOf(const Expr& e);

struct FieldExpr {
    std::string identifier;

    ValueType valueType() const { return ValueType::String; }
    ExprKindLevel level() const { return ExprKindLevel::Row; }
};

struct ValueExpr {
    Value value;

    ValueType valueType() const { return value.type(); }
    ExprKindLevel level() const { return ExprKindLevel::Const; }
};

struct InExpr {
    Box<Expr> expr;
    Box<Relation> source;

    ValueType valueType() const { return ValueType::Boolean; }
    ExprKindLevel level() const { return ExprKindLevel::Row; }
};

struct CoalesceExpr {
    std::vector<Expr> args;

    ValueType valueType() const;
    ExprKindLevel level() const;
};

struct CastExpr {
    ValueType cast_to;
    Box<Expr> expr;

    ValueType valueType() const { return cast_to; }
    ExprKindLevel level() const;
};

struct PercentileExpr {
    Box<Expr> expr;
    std::vector<float> percentiles;

    ValueType valueType() const { return ValueType::String; }
    ExprKindLevel level() const { return ExprKindLevel::Group; }
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;

    ValueType valueType() const { return ValueType::Boolean; }
    ExprKindLevel level() const;
};

struct RSubstrExpr {
    Box<Expr> expr;
    std::string regex;

    ValueType valueType() const { return ValueType::String; }
    ExprKindLevel level() const;
};

enum class UnaryExprType {
    BooleanNegate,
};

struct UnaryExpr {
    UnaryExprType type;
    ValueType value_type;
    Box<Expr> expr;

    ValueType valueType() const { return value_type; }
    ExprKindLevel level() const;
};

enum class UnaryAggregateExprType {
    Count,
    Min,
    Max,
    Sum,
};

struct UnaryAggregateExpr {
    UnaryAggregateExprType type;
    ValueType value_type;
    Box<Expr> expr;

    ValueType valueType() const { return value_type; }
    ExprKindLevel level() const { return ExprKindLevel::Group; }
};

enum class BinaryExprType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
};

struct BinaryExpr {
    BinaryExprType type;
    ValueType value_type;
    Box<Expr> left;
    Box<Expr> right;

    ValueType valueType() const { return value_type; }
    ExprKindLevel level() const;
};

}  // namespace lsql::iface::sql::bind
