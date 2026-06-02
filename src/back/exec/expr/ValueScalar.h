#pragma once

#include "back/exec/expr/Scalar.h"

namespace lsql::back::exec {

class ValueScalar : public Scalar {
 public:
    explicit ValueScalar(Value value) : value_(std::move(value)) {}
    const Value& get() const { return value_; }

    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    ValueType valueType() const override { return value_.type(); }
    Value eval(const back::exec::Record& /*record*/) const override { return value_; }

 private:
    Value value_;
};

}  // namespace lsql::back::exec
