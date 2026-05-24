#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class ValueExpression : public Expression {
 public:
    explicit ValueExpression(Value value) : value_(std::move(value)) {}

    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    const Value& get() const { return value_; }
    ValueType valueType() const override { return value_.type(); }
    Value eval(const exec::Record& /*record*/) const override { return value_; }

 private:
    Value value_;
};

}  // namespace lsql::exec
