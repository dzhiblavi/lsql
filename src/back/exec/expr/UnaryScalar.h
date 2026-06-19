#pragma once

#include "back/exec/expr/Scalar.h"

namespace lsql::back::exec {

template <typename Op>
concept UnaryOperation = requires(const Op& op, Value val) {
    { op.apply(std::move(val)) } -> std::same_as<Value>;
    { op.valueType() } -> std::same_as<ValueType>;
    { op.argType() } -> std::same_as<ValueType>;
};

template <UnaryOperation Op>
class UnaryScalar : public Scalar {
 public:
    template <typename... Args>
    explicit UnaryScalar(ScalarPtr arg, Args&&... args)
        : arg_(std::move(arg))
        , op_(std::forward<Args>(args)...) {
        verify(arg_->valueType() == op_.argType());
    }

    FieldSet requiredFields() const override { return arg_->requiredFields(); }

    ValueType valueType() const override { return op_.valueType(); }

    Value eval(const back::exec::Record& record) const override {
        return op_.apply(arg_->eval(record));
    }

 private:
    ScalarPtr arg_;
    [[no_unique_address]] Op op_;
};

struct BooleanNegationOp {
    Value apply(Value val) const { return !val.get<bool>(); }
    ValueType valueType() const { return ValueType::Boolean; }
    ValueType argType() const { return ValueType::Boolean; }
};

}  // namespace lsql::back::exec
