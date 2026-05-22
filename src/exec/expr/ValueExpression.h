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
    Value eval(const std::vector<exec::ConstRecordPtr>& /*group*/) const override { return value_; }
    AggregatorPtr aggregator() const override { throw std::runtime_error("cannot aggregate"); }

 private:
    Value value_;
};

}  // namespace lsql::exec
