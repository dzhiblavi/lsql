#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

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
class BinaryExpression : public Expression {
    struct Aggr : Aggregator {
        Aggr(AggregatorPtr al, AggregatorPtr ar, const Op* op) : al(al), ar(ar), op(op) {}

        void feed(const exec::Record& rec) override {
            al->feed(rec);
            ar->feed(rec);
        }

        Value get() override { return op->apply(al->get(), ar->get()); }

        AggregatorPtr al;
        AggregatorPtr ar;
        const Op* op;
    };

 public:
    template <typename... Args>
    BinaryExpression(ExpressionPtr l, ExpressionPtr r, Args&&... args)
        : l_(std::move(l))
        , r_(std::move(r))
        , op_(std::forward<Args>(args)...) {
        if (l_->valueType() != op_.argTypeL() || r_->valueType() != op_.argTypeR()) {
            throw std::runtime_error("argument type mismatch");
        }
    }

    ValueType valueType() const override { return op_.valueType(); }

    AggregatorPtr aggregator() const override {
        return std::make_shared<Aggr>(l_->aggregator(), r_->aggregator(), &op_);
    }

    Value eval(const exec::Record& record) const override {
        return op_.apply(l_->eval(record), r_->eval(record));
    }

    Value eval(const std::vector<exec::ConstRecordPtr>& group) const override {
        return op_.apply(l_->eval(group), r_->eval(group));
    }

 private:
    ExpressionPtr l_, r_;
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

struct DivideOp {
    Value apply(const Value& l, const Value& r) const {
        return visit(
            util::Overloaded{
                [](int64_t a, int64_t b) -> Value { return a / b; },
                [](float a, float b) -> Value { return a / b; },
                [](auto...) -> Value {
                    assert(false);
                    throw std::runtime_error("invalid argument types");
                },
            },
            l,
            r);
    }

    ValueType argTypeL() const { return type; }
    ValueType argTypeR() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

}  // namespace lsql::exec
