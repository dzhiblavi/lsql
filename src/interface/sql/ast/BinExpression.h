#pragma once

#include "core/ValueType.h"
#include "interface/sql/ast/Expression.h"

namespace lsql::iface::sql::ast {

enum class BinExpressionType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
};

ValueType binExprResultType(BinExpressionType type, ValueType l, ValueType r);

class BinaryExpression : public Expression {
 public:
    BinaryExpression(
        std::unique_ptr<Expression> l, std::unique_ptr<Expression> r, BinExpressionType type)
        : Expression(
              composed(l->type(), r->type()),
              binExprResultType(type, l->valueType(), r->valueType()))
        , bin_type(type)
        , l(std::move(l))
        , r(std::move(r)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    BinExpressionType bin_type;
    std::unique_ptr<Expression> l;
    std::unique_ptr<Expression> r;
};

}  // namespace lsql::iface::sql::ast
