#pragma once

#include "back/exec/expr/Scalar.h"
#include "core/function/Executor.h"

namespace lsql::back::exec {

template <func::BinaryExecutor E>
class BinaryFnScalar : public Scalar {
 public:
    BinaryFnScalar(ValueType value_type, E executor, Arc<Scalar> left, Arc<Scalar> right)
        : value_type_(value_type)
        , left_(std::move(left))
        , right_(std::move(right))
        , executor_(std::move(executor)) {}

    FieldSet requiredFields() const override {
        return FieldSet::merge(left_->requiredFields(), right_->requiredFields());
    }

    ValueType valueType() const override { return value_type_; }

    Value eval(const back::exec::Record& record) const override {
        return executor_.execute(left_->eval(record), right_->eval(record));
    }

 private:
    ValueType value_type_;
    Arc<Scalar> left_;
    Arc<Scalar> right_;
    [[no_unique_address]] E executor_;
};

}  // namespace lsql::back::exec
