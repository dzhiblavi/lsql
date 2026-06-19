#pragma once

#include "back/exec/expr/Scalar.h"
#include "core/function/Executor.h"

namespace lsql::back::exec {

template <func::UnaryExecutor E>
class UnaryFnScalar : public Scalar {
 public:
    UnaryFnScalar(ValueType value_type, E executor, Arc<Scalar> arg)
        : value_type_(value_type)
        , arg_(std::move(arg))
        , executor_(std::move(executor)) {}

    FieldSet requiredFields() const override { return arg_->requiredFields(); }
    ValueType valueType() const override { return value_type_; }

    Value eval(const back::exec::Record& record) const override {
        return executor_.execute(arg_->eval(record));
    }

 private:
    ValueType value_type_;
    Arc<Scalar> arg_;
    [[no_unique_address]] E executor_;
};

}  // namespace lsql::back::exec
