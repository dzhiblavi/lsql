#pragma once

#include "core/ValueType.h"
#include "sql/ast/Expression.h"

namespace lsql::sql::ast {

enum class UnaryExpressionType {
    BooleanNegate,
};

ValueType unaryExprResultType(UnaryExpressionType type, ValueType a);

class UnaryExpression : public Expression {
 public:
    UnaryExpression(std::unique_ptr<Expression> a, UnaryExpressionType type)
        : Expression(a->type(), unaryExprResultType(type, a->valueType()))
        , un_type(type)
        , a(std::move(a)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    UnaryExpressionType un_type;
    std::unique_ptr<Expression> a;
};

}  // namespace lsql::sql::ast
