#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class IdentifierExpression : public Expression {
 public:
    explicit IdentifierExpression(std::string name) : name_(std::move(name)) {}

    ValueType valueType() const override { return ValueType::String; }

    Value eval(const rel::Record& record) const override { return record.value(name_); }

    Value eval(const std::vector<rel::ConstRecordPtr>& group) const override {
        return eval(*group.front());
    }

    AggregatorPtr aggregator() const override {
        assert(false);
        throw std::runtime_error("not implemented");
    }

 private:
    std::string name_;
};

}  // namespace lsql::exec
