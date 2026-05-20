#pragma once

#include "exec/expr/Expression.h"
#include "exec/prof/Metrics.h"
#include "exec/prof/OperationHandle.h"
#include "util/instrument/Timer.h"

#include <llog/log.h>

#include <algorithm>
#include <sstream>

namespace lsql::exec {

template <typename Op>
concept UnaryAggregateOperation = requires(Op op, Value val, Op::State* state) {
    typename Op::State;
    { op.update(state, val) } -> std::same_as<void>;
    { op.result(state) } -> std::same_as<Value>;
    { op.valueType() } -> std::same_as<ValueType>;
    { op.argType() } -> std::same_as<ValueType>;
};

template <UnaryAggregateOperation Op>
class UnaryAggregateExpression : public Expression {
    struct Aggr : Aggregator {
        Aggr(ExpressionPtr expr, const Op* op) : expr(expr), op(op) {}
        void feed(const exec::Record& record) override { op->update(&state, expr->eval(record)); }
        Value get() override { return op->result(&state); }

        ExpressionPtr expr;
        const Op* op;
        typename Op::State state;
    };

 public:
    template <typename... Args>
    explicit UnaryAggregateExpression(ExpressionPtr arg, Args&&... args)
        : arg_(std::move(arg))
        , op_(std::forward<Args>(args)...) {
        if (arg_->valueType() != op_.argType()) {
            throw std::runtime_error("argument type mismatch");
        }
    }

    RequiredFields requiredFields() const override { return arg_->requiredFields(); }

    ValueType valueType() const override { return op_.valueType(); }

    AggregatorPtr aggregator() const override { return std::make_shared<Aggr>(arg_, &op_); }

    Value eval(const exec::Record& /*record*/) const override {
        panic("aggregate expression should not be called on row basis");
    }

    Value eval(const std::vector<exec::ConstRecordPtr>& group) const override {
        typename Op::State state;
        for (auto&& record : group) {
            op_.update(&state, arg_->eval(*record));
        }
        return op_.result(&state);
    }

 private:
    ExpressionPtr arg_;
    [[no_unique_address]] Op op_;
};

struct CountOp {
    using State = int64_t;

    void update(State* curr, const Value& condition) const {
        if (condition.get<bool>()) {
            ++*curr;
        }
    }

    Value result(const State* state) const { return *state; }
    ValueType argType() const { return ValueType::Boolean; }
    ValueType valueType() const { return ValueType::Integer; }
};

struct MinOp {
    using State = Value;

    void update(State* curr, const Value& value) const {
        if (*curr == null) {
            *curr = value;
        } else {
            *curr = std::min(*curr, value);
        }
    }

    Value result(const State* state) const { return *state; }
    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

struct MaxOp {
    using State = Value;

    void update(State* curr, const Value& value) const {
        if (*curr == null) {
            *curr = value;
        } else {
            *curr = std::max(*curr, value);
        }
    }

    Value result(const State* state) const { return *state; }
    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

struct SumOp {
    using State = Value;

    void update(State* curr, const Value& value) const {
        if (*curr == null) {
            *curr = value;
            return;
        }

        *curr = visit(
            util::Overloaded{
                [](int64_t a, int64_t b) -> Value { return a + b; },
                [](float a, float b) -> Value { return a + b; },
                [](auto...) -> Value {
                    assert(false);
                    throw std::runtime_error("invalid argument types");
                }},
            *curr,
            value);
    }

    Value result(const State* state) const { return *state; }
    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

struct PercentileOp {
    struct State {
        State() : info(prof::currentOperation().addTransientMetric<prof::Message>()) {}

        std::vector<Value> values;
        prof::Message* info;
    };

    PercentileOp(std::vector<float> perc, ValueType type)
        : percentiles([&](auto p) {
            std::sort(p.begin(), p.end());
            return p;
        }(std::move(perc)))
        , type(type) {}

    void update(State* curr, const Value& value) const { curr->values.push_back(value); }

    Value result(State* state) const {
        std::vector<float> result;
        result.reserve(percentiles.size());

        std::ptrdiff_t size = static_cast<std::ptrdiff_t>(state->values.size());
        auto left = state->values.begin();

        auto get = [](auto&& v) {
            return visit(
                util::Overloaded{
                    [](float x) -> float { return x; },
                    [](int64_t x) -> float { return float(x); },
                    [](auto&&) -> float {
                        assert(false);
                        throw std::runtime_error("invalid types");
                    },
                },
                v);
        };

        constexpr float threshold = 1e-6;

        instr::Timer timer;

        for (float p : percentiles) {
            auto pos = [&] -> std::ptrdiff_t {
                if (abs(p) < threshold) {
                    return 0;
                }
                if (abs(p - 1.f) < threshold) {
                    return size - 1;
                }
                return std::min(
                    size - 1, static_cast<std::ptrdiff_t>(static_cast<float>(size) * p));
            }();

            auto it = std::next(state->values.begin(), pos);
            assert(left <= it);

            std::nth_element(left, it, state->values.end(), [](auto&& l, auto&& r) {
                return visit(
                    util::Overloaded{
                        [](float x, float y) { return x < y; },
                        [](int64_t x, int64_t y) { return x < y; },
                        [](auto&&...) -> bool {
                            assert(false);
                            throw std::runtime_error("invalid types");
                        },
                    },
                    l,
                    r);
            });

            result.push_back(get(*it));
        }

        if (result.empty()) {
            return "";
        }

        if (state->info) {
            state->info->set(
                "percentile size: {}, percentiles: {}, time={}",
                state->values.size(),
                percentiles.size(),
                instr::prettyDuration(timer.elapsed()));
        }

        std::stringstream ss;
        ss << '[';
        for (float p : result) {
            ss << p << ", ";
        }
        ss.seekp(-2, std::ios_base::end);  // remove last ', '
        ss << ']';
        return ss.str();
    }

    ValueType argType() const { return type; }
    ValueType valueType() const { return ValueType::String; }

    std::vector<float> percentiles;
    ValueType type;
};

}  // namespace lsql::exec
