#pragma once

#include "back/exec/expr/Scalar.h"

#include "core/value/valueCast.h"

#include <reflex/stdmatcher.h>

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

struct LikeOp {
    explicit LikeOp(const std::string& regex) : pattern(regex) {}

    Value apply(const Value& value) const {
        auto view = value.get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        return reflex::Matcher(&pattern, input).matches() != 0;
    }

    ValueType valueType() const { return ValueType::Boolean; }
    ValueType argType() const { return ValueType::String; }

    reflex::Pattern pattern;
};

struct RSubstrOp {
    explicit RSubstrOp(const std::string& regex) : pattern(regex) {}

    Value apply(const Value& value) const {
        auto view = value.get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        reflex::Matcher matcher(&pattern, input);

        size_t group = matcher.find();
        if (group == 0) {
            return null;
        }

        return value.substr(matcher.first(), matcher.size());
    }

    ValueType valueType() const { return ValueType::String; }
    ValueType argType() const { return ValueType::String; }

    const reflex::Pattern pattern;
};

struct CastOp {
    ValueType valueType() const { return to; }
    ValueType argType() const { return from; }
    Value apply(Value val) const { return valueCast(std::move(val), to).value_or(null); }

    ValueType from, to;
};

}  // namespace lsql::back::exec
