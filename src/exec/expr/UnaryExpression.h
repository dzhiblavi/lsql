#pragma once

#include "exec/expr/Expression.h"

#include <reflex/stdmatcher.h>

namespace lsql::exec {

template <typename Op>
concept UnaryOperation = requires(const Op& op, Value val) {
    { op.apply(std::move(val)) } -> std::same_as<Value>;
    { op.valueType() } -> std::same_as<ValueType>;
    { op.argType() } -> std::same_as<ValueType>;
};

template <UnaryOperation Op>
class UnaryExpression : public Expression {
    struct Aggr : Aggregator {
        Aggr(AggregatorPtr arg, const Op* op) : arg(arg), op(op) {}
        void feed(const exec::Record& rec) override { arg->feed(rec); }
        Value get() override { return op->apply(arg->get()); }

        AggregatorPtr arg;
        const Op* op;
    };

 public:
    template <typename... Args>
    explicit UnaryExpression(ExpressionPtr arg, Args&&... args)
        : arg_(std::move(arg))
        , op_(std::forward<Args>(args)...) {
        if (arg_->valueType() != op_.argType()) {
            throw std::runtime_error("argument type mismatch");
        }
    }

    ValueType valueType() const override { return op_.valueType(); }

    AggregatorPtr aggregator() const override {
        return std::make_shared<Aggr>(arg_->aggregator(), &op_);
    }

    Value eval(const exec::Record& record) const override { return op_.apply(arg_->eval(record)); }

    Value eval(const std::vector<exec::ConstRecordPtr>& group) const override {
        return op_.apply(arg_->eval(group));
    }

 private:
    ExpressionPtr arg_;
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
        return reflex::Matcher(&pattern, value.get<std::string>()).matches() != 0;
    }

    ValueType valueType() const { return ValueType::Boolean; }
    ValueType argType() const { return ValueType::String; }

    reflex::Pattern pattern;
};

struct RSubstrOp {
    explicit RSubstrOp(const std::string& regex) : pattern(regex) {}

    Value apply(const Value& value) const {
        auto val = value.get<std::string>();
        reflex::Matcher matcher(&pattern, val);

        size_t group = matcher.find();
        if (group == 0) {
            return null;
        }

        return val.substr(matcher.first(), matcher.size());
    }

    ValueType valueType() const { return ValueType::String; }
    ValueType argType() const { return ValueType::String; }

    const reflex::Pattern pattern;
};

struct CastOp {
    ValueType valueType() const { return to; }
    ValueType argType() const { return from; }

    Value apply(Value val) const {
        return std::move(val).visit(
            util::Overloaded{
                [this](null_t) -> Value {
                    switch (to) {
                        case ValueType::String:
                            return std::string();
                        case ValueType::Integer:
                            return int64_t(0);
                        case ValueType::Boolean:
                            return false;
                        case ValueType::Floating:
                            return 0.f;
                        case ValueType::Null:
                            return null;
                    };
                },
                [this](const std::string& s) -> Value {
                    switch (to) {
                        case ValueType::String:
                            return s;
                        case ValueType::Integer:
                            return int64_t(std::stoll(s));
                        case ValueType::Boolean:
                            return !s.empty();
                        case ValueType::Floating:
                            return float(std::strtof(s.data(), nullptr));
                        case ValueType::Null:
                            return null;
                    };
                },
                [this](int64_t x) -> Value {
                    switch (to) {
                        case ValueType::String:
                            return std::to_string(x);
                        case ValueType::Integer:
                            return x;
                        case ValueType::Boolean:
                            return x != 0;
                        case ValueType::Floating:
                            return float(x);
                        case ValueType::Null:
                            return null;
                    };
                },
                [this](float x) -> Value {
                    switch (to) {
                        case ValueType::String:
                            return std::to_string(x);
                        case ValueType::Integer:
                            return int64_t(x);
                        case ValueType::Boolean:
                            return x != 0.f;
                        case ValueType::Floating:
                            return x;
                        case ValueType::Null:
                            return null;
                    };
                },
                [this](bool x) -> Value {
                    switch (to) {
                        case ValueType::String:
                            return x ? "true" : "false";
                        case ValueType::Integer:
                            return int64_t(x ? 1 : 0);
                        case ValueType::Boolean:
                            return x;
                        case ValueType::Floating:
                            return x ? 1.f : 0.f;
                        case ValueType::Null:
                            return null;
                    };
                },
            });
    }

    ValueType from, to;
};

}  // namespace lsql::exec
