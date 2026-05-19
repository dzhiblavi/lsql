#pragma once

#include "core/ValueType.h"
#include "interface/sql/ast/Expression.h"

namespace lsql::sql::ast {

enum class UnaryAggregateType {
    Count,
    Min,
    Max,
    Sum,
};

ValueType unaryAggregateResultType(UnaryAggregateType type, ValueType a);

class UnaryAggregateExpression : public Expression {
 public:
    UnaryAggregateExpression(std::unique_ptr<Expression> cond, UnaryAggregateType type)
        : Expression(ExpressionType::Group, unaryAggregateResultType(type, cond->valueType()))
        , condition(std::move(cond))
        , type(type) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> condition;
    UnaryAggregateType type;
};

}  // namespace lsql::sql::ast
