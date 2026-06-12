#pragma once

#include "back/exec/expr/Aggregate.h"
#include "back/exec/expr/Scalar.h"

#include "core/exprs/concepts.h"
#include "util/instrument/Timer.h"
#include "util/require.h"

#include <llog/log.h>

#include <algorithm>
#include <sstream>

namespace lsql::back::exec {

template <typename Op>
concept UnaryAggregateOperation = requires(Op op, Value val, Op::State* state) {
    typename Op::State;
    { op.update(state, val) } -> std::same_as<void>;
    { op.result(state) } -> std::same_as<Value>;
    { op.valueType() } -> std::same_as<ValueType>;
    { op.argType() } -> std::same_as<ValueType>;
};

template <UnaryAggregateOperation Op>
class UnaryAggregate : public Aggregate {
    struct Aggr : Aggregator {
        Aggr(ScalarPtr expr, const Op* op) : expr(expr), op(op) {}
        void feed(const back::exec::Record& record) override {
            op->update(&state, expr->eval(record));
        }
        Value get() override { return op->result(&state); }

        ScalarPtr expr;
        const Op* op;
        typename Op::State state{};
    };

 public:
    template <typename... Args>
    explicit UnaryAggregate(ScalarPtr arg, Args&&... args)
        : arg_(std::move(arg))
        , op_(std::forward<Args>(args)...) {
        require(arg_->valueType() == op_.argType(), "wrong argument type");
    }

    FieldSet requiredFields() const override { return arg_->requiredFields(); }
    ValueType valueType() const override { return op_.valueType(); }
    AggregatorPtr aggregator() const override { return arc<Aggr>(arg_, &op_); }

 private:
    ScalarPtr arg_;
    [[no_unique_address]] Op op_;
};

struct CountNonNullOp {
    using State = int64_t;

    void update(State* curr, const Value& value) const {
        if (value != null) {
            ++*curr;
        }
    }

    Value result(const State* state) const { return *state; }
    ValueType argType() const { return arg_type; }
    ValueType valueType() const { return ValueType::Integer; }

    ValueType arg_type;
};

template <Comparable T>
struct MinOp {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;
    using State = std::optional<U>;

    void update(State* curr, const Value& value) const {
        if (value == null) {
            return;
        }

        auto&& next = value.get<T>();
        if (curr->has_value()) {
            *curr = std::min(**curr, U(next));
        } else {
            *curr = U(next);
        }
    }

    Value result(const State* state) const {
        if (state->has_value()) {
            return std::move(**state);
        }

        return null;
    }

    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

template <Comparable T>
struct MaxOp {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;
    using State = std::optional<U>;

    void update(State* curr, const Value& value) const {
        if (value == null) {
            return;
        }

        auto&& next = value.get<T>();
        if (curr->has_value()) {
            *curr = std::max(**curr, U(next));
        } else {
            *curr = U(next);
        }
    }

    Value result(const State* state) const {
        if (state->has_value()) {
            return std::move(**state);
        }

        return null;
    }

    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    ValueType type;
};

template <Addable T>
struct SumOp {
    using State = std::conditional_t<std::same_as<T, std::string_view>, std::stringstream, T>;

    void update(State* curr, const Value& value) const {
        if (value != null) {
            add(*curr, value.get<T>());
        }
    }

    ValueType argType() const { return type; }
    ValueType valueType() const { return type; }

    template <typename U = T>
    requires(std::same_as<U, std::string_view>)
    Value result(const State* state) const {
        return state->str();
    }

    template <typename U = T>
    requires(!std::same_as<U, std::string_view>)
    Value result(const State* state) const {
        return *state;
    }

    template <typename U = T>
    requires(std::same_as<U, std::string_view>)
    void add(State& curr, std::string_view s) const {
        curr << s;
    }

    template <typename U = T>
    requires(!std::same_as<U, std::string_view>)
    void add(State& curr, const U& s) const {
        curr += s;
    }

    ValueType type;
};

template <Comparable T>
struct PercentileOp {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    struct State {
        std::vector<U> values;
    };

    PercentileOp(std::vector<float> perc, ValueType type)
        : percentiles([&](auto p) {
            std::sort(p.begin(), p.end());
            return p;
        }(std::move(perc)))
        , type(type) {}

    void update(State* curr, const Value& value) const {
        if (value != null) {
            curr->values.emplace_back(value.get<T>());
        }
    }

    Value result(State* state) const {
        std::vector<U> result;
        result.reserve(percentiles.size());

        std::ptrdiff_t size = static_cast<std::ptrdiff_t>(state->values.size());
        auto left = state->values.begin();

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

            std::nth_element(left, it, state->values.end());
            result.push_back(*it);
        }

        if (result.empty()) {
            return std::string("");
        }

        std::stringstream ss;
        ss << '[';
        for (auto&& p : result) {
            ss << p << ',';
        }
        ss.seekp(-1, std::ios_base::end);  // remove last ','
        ss << ']';
        return ss.str();
    }

    ValueType argType() const { return type; }
    ValueType valueType() const { return ValueType::String; }

    std::vector<float> percentiles;
    ValueType type;
};

}  // namespace lsql::back::exec
