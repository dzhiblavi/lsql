#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class ValueExpression : public Expression {
    struct Aggr : Aggregator {
        explicit Aggr(Value value) : value_(std::move(value)) {}
        Value get() override { return std::move(value_); }
        void feed(const exec::Record& /*record*/) override { /*nothing*/ }
        Value value_;
    };

 public:
    explicit ValueExpression(Value value) : value_(std::move(value)) {}

    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    const Value& get() const { return value_; }
    ValueType valueType() const override { return value_.type(); }
    Value eval(const exec::Record& /*record*/) const override { return value_; }
    Value eval(const std::vector<exec::ConstRecordPtr>& /*group*/) const override { return value_; }
    AggregatorPtr aggregator() const override { return std::make_shared<Aggr>(value_); }

 private:
    Value value_;
};

}  // namespace lsql::exec
