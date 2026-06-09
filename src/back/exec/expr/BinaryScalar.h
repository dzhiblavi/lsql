#pragma once

#include "back/exec/expr/Scalar.h"

#include "core/exprs/concepts.h"

namespace lsql::back::exec {

template <typename Op>
concept BinaryOperation =
    std::is_default_constructible_v<Op> && std::is_trivially_copyable_v<Op> &&
    requires(const Op& op, Value val) {
        { op.apply(std::move(val), std::move(val)) } -> std::same_as<Value>;
        { op.valueType() } -> std::same_as<ValueType>;
        { op.argTypeL() } -> std::same_as<ValueType>;
        { op.argTypeR() } -> std::same_as<ValueType>;
    };

template <BinaryOperation Op>
class BinaryScalar : public Scalar {
 public:
    template <typename... Args>
    BinaryScalar(ScalarPtr l, ScalarPtr r, Args&&... args)
        : l_(std::move(l))
        , r_(std::move(r))
        , op_(std::forward<Args>(args)...) {
        verify(l_->valueType() == op_.argTypeL() && r_->valueType() == op_.argTypeR());
    }

    FieldSet requiredFields() const override {
        return FieldSet::merge(l_->requiredFields(), r_->requiredFields());
    }

    ValueType valueType() const override { return op_.valueType(); }

    Value eval(const back::exec::Record& record) const override {
        return op_.apply(l_->eval(record), r_->eval(record));
    }

 private:
    ScalarPtr l_, r_;
    [[no_unique_address]] Op op_;
};

struct EqualOp {
    Value apply(const Value& l, const Value& r) const { return l == r; }
    ValueType argTypeL() const { return l; }
    ValueType argTypeR() const { return r; }
    ValueType valueType() const { return ValueType::Boolean; }
    ValueType l, r;
};

struct NotEqualOp {
    Value apply(const Value& l, const Value& r) const { return l != r; }
    ValueType argTypeL() const { return l; }
    ValueType argTypeR() const { return r; }
    ValueType valueType() const { return ValueType::Boolean; }
    ValueType l, r;
};

struct AndOp {
    Value apply(const Value& l, const Value& r) const { return l.get<bool>() && r.get<bool>(); }
    ValueType argTypeL() const { return ValueType::Boolean; }
    ValueType argTypeR() const { return ValueType::Boolean; }
    ValueType valueType() const { return ValueType::Boolean; }
};

struct OrOp {
    Value apply(const Value& l, const Value& r) const { return l.get<bool>() || r.get<bool>(); }
    ValueType argTypeL() const { return ValueType::Boolean; }
    ValueType argTypeR() const { return ValueType::Boolean; }
    ValueType valueType() const { return ValueType::Boolean; }
};

template <Dividable T>
struct DivideOp {
    Value apply(const Value& l, const Value& r) const {
        auto divisor = r.get<T>();
        return divisor == T(0) ? null : Value(l.get<T>() / divisor);
    }
    ValueType argTypeL() const { return type; }
    ValueType argTypeR() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

template <Addable T>
struct AddOp {
    Value apply(const Value& l, const Value& r) const { return l.get<T>() + r.get<T>(); }
    ValueType argTypeL() const { return type; }
    ValueType argTypeR() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

template <Subtractable T>
struct SubtractOp {
    Value apply(const Value& l, const Value& r) const { return l.get<T>() - r.get<T>(); }
    ValueType argTypeL() const { return type; }
    ValueType argTypeR() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

}  // namespace lsql::back::exec
