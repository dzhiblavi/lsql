#pragma once

#include "core/ValueType.h"
#include "interface/sql/ast/ExpressionType.h"
#include "interface/sql/ast/Node.h"
#include "interface/sql/ast/Visitor.h"

#include <memory>
#include <string>
#include <vector>

namespace lsql::sql::ast {

class Expression : public sql::ast::Node {
 public:
    Expression(ExpressionType type, ValueType value_type) : type_(type), value_type_(value_type) {}

    ExpressionType type() const { return type_; }
    ValueType valueType() const { return value_type_; }

 private:
    ExpressionType type_;
    ValueType value_type_;
};

using ExpressionList = std::vector<std::unique_ptr<Expression>>;

class IdentifierExpression : public Expression {
 public:
    IdentifierExpression(std::string id, ValueType value_type)
        : Expression(ExpressionType::Row, value_type)
        , id(std::move(id)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string id;
};

class ValueExpression : public Expression {
 public:
    ValueExpression(std::string val, ValueType value_type)
        : Expression(ExpressionType::Const, value_type)
        , value_str(std::move(val)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string value_str;
};

class CastExpression : public Expression {
 public:
    CastExpression(std::unique_ptr<Expression> e, ValueType value_type)
        : Expression(e->type(), value_type)
        , expr(std::move(e)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> expr;
};

class InExpression : public Expression {
 public:
    InExpression(std::unique_ptr<Expression> left, std::unique_ptr<Node> source)
        : Expression(ExpressionType::Row, ValueType::Boolean)
        , left(std::move(left))
        , source(std::move(source)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> left;
    std::unique_ptr<Node> source;
};

class PercentileExpression : public Expression {
 public:
    PercentileExpression(std::unique_ptr<Expression> value, std::vector<float>* percentiles)
        : Expression(
              ExpressionType::Group,
              ValueType::String)  // for now the result vector is just formatted to a string
        , value(std::move(value))
        , percentiles(percentiles) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> value;
    std::unique_ptr<std::vector<float>> percentiles;
};

class LikeExpression : public Expression {
 public:
    LikeExpression(std::unique_ptr<Expression> a, std::string regex)
        : Expression(a->type(), ValueType::Boolean)
        , a(std::move(a))
        , regex(std::move(regex)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> a;
    std::string regex;
};

class RSubstrExpression : public Expression {
 public:
    RSubstrExpression(std::unique_ptr<Expression> a, std::string regex)
        : Expression(a->type(), ValueType::String)
        , arg(std::move(a))
        , regex(std::move(regex)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> arg;
    std::string regex;
};

class CoalesceExpression : public Expression {
 public:
    explicit CoalesceExpression(std::vector<std::unique_ptr<Expression>>* values)
        : Expression(values->front()->type(), values->front()->valueType())
        , values(std::move(values)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<std::vector<std::unique_ptr<Expression>>> values;
};

class AdhocRelation : public Node {
 public:
    AdhocRelation(std::unique_ptr<std::vector<std::unique_ptr<ValueExpression>>> values)
        : values(std::move(values)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<std::vector<std::unique_ptr<ValueExpression>>> values;
};

}  // namespace lsql::sql::ast
