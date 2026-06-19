#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Executor.h"
#include "core/function/Function.h"

#include "core/exprs/concepts.h"
#include "core/value/cast.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>
#include <reflex/stdmatcher.h>

#include <algorithm>

namespace lsql::func {

struct SubstrExecutor;
struct CoalesceExecutor;
struct RSubstrExecutor;
struct LikeExecutor;
struct CastExecutor;

struct CountNonNullAggregate;
struct CountAllAggregate;

template <Comparable T>
struct PercentileAggregate;

template <Comparable T>
struct MinAggregate;

template <Comparable T>
struct MaxAggregate;

template <Addable T>
struct SumAggregate;

SubstrExecutor build(const Substr& s);
CoalesceExecutor build(const Coalesce& s);
RSubstrExecutor build(const RSubstr& s);
LikeExecutor build(const Like& s);
CastExecutor build(const Cast& s);

template <typename T>
PercentileAggregate<T> build(const Percentile& s);

CountNonNullAggregate build(const CountNonNull& s);
CountAllAggregate build(const CountAll& s);

template <typename T>
MinAggregate<T> build(const Min& s);

template <typename T>
MaxAggregate<T> build(const Max& s);

template <typename T>
SumAggregate<T> build(const Sum& s);

struct SubstrExecutor {
    SubstrExecutor(size_t from, size_t len) : from(from), length(len) {}

    Value execute(std::span<Value> values) const {
        verify_dbg(values.size() == 1);
        return values[0].substr(from, length);
    }

    size_t from;
    size_t length;
};

static_assert(Executor<SubstrExecutor>);

struct CoalesceExecutor {
    Value execute(std::span<Value> values) const {
        for (auto&& value : values) {
            if (value != vnull) {
                return value;
            }
        }
        return vnull;
    }
};

static_assert(Executor<CoalesceExecutor>);

struct RSubstrExecutor {
    explicit RSubstrExecutor(const std::string& regex) : regex(regex), pattern(regex) {}
    RSubstrExecutor(const RSubstrExecutor& e) : RSubstrExecutor(e.regex) {}

    Value execute(std::span<Value> values) const {
        verify_dbg(values.size() == 1);

        auto view = values[0].get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        reflex::Matcher matcher(&pattern, input);

        size_t group = matcher.find();
        if (group == 0) {
            return null;
        }

        return values[0].substr(matcher.first(), matcher.size());
    }

    std::string regex;
    reflex::Pattern pattern;
};

static_assert(Executor<RSubstrExecutor>);

struct LikeExecutor {
    explicit LikeExecutor(const std::string& regex) : pattern(regex) {}

    Value execute(std::span<Value> values) const {
        verify_dbg(values.size() == 1);
        auto view = values[0].get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        return reflex::Matcher(&pattern, input).matches() != 0;
    }

    const reflex::Pattern pattern;
};

static_assert(Executor<LikeExecutor>);

struct CastExecutor {
    explicit CastExecutor(ValueType cast_to) : cast_to(cast_to) {}

    Value execute(std::span<Value> values) const {
        verify_dbg(values.size() == 1);
        return cast(std::move(values[0]), cast_to).value_or(null);
    }

    ValueType cast_to;
};

static_assert(Executor<CastExecutor>);

struct CountNonNullAggregator {
    void feed(std::span<Value> values) {
        verify_dbg(values.size() == 1);
        count += (values[0] != vnull);
    }

    Value get() { return count; }

    int64_t count = 0;
};

static_assert(Aggregator<CountNonNullAggregator>);

struct CountNonNullAggregate {
    using Aggregator = CountNonNullAggregator;
    CountNonNullAggregator aggregator() const { return {}; }
};

static_assert(Aggregate<CountNonNullAggregate>);

struct CountAllAggregator {
    void feed(std::span<Value> /*values*/) { ++count; }
    Value get() { return count; }
    int64_t count = 0;
};

static_assert(Aggregator<CountAllAggregator>);

struct CountAllAggregate {
    using Aggregator = CountAllAggregator;
    CountAllAggregator aggregator() const { return {}; }
};

static_assert(Aggregate<CountAllAggregate>);

template <typename T>
struct MinAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(std::span<Value> values) {
        verify_dbg(values.size() == 1);
        if (values[0] == vnull) {
            return;
        }

        auto&& next = values[0].get<T>();
        if (res.has_value()) {
            res = std::min(*res, U(next));
        } else {
            res = U(next);
        }
    }

    Value get() {
        if (res.has_value()) {
            return std::move(*res);
        }
        return vnull;
    }

    std::optional<U> res = std::nullopt;
};

static_assert(Aggregator<MinAggregator<int64_t>>);

template <Comparable T>
struct MinAggregate {
    using Aggregator = MinAggregator<T>;
    MinAggregator<T> aggregator() const { return {}; }
};

static_assert(Aggregate<MinAggregate<int64_t>>);

template <typename T>
struct MaxAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(std::span<Value> values) {
        verify_dbg(values.size() == 1);
        if (values[0] == vnull) {
            return;
        }

        auto&& next = values[0].get<T>();
        if (res.has_value()) {
            res = std::max(*res, U(next));
        } else {
            res = U(next);
        }
    }

    Value get() {
        if (res.has_value()) {
            return std::move(*res);
        }
        return vnull;
    }

    std::optional<U> res = std::nullopt;
};

static_assert(Aggregator<MaxAggregator<int64_t>>);

template <Comparable T>
struct MaxAggregate {
    using Aggregator = MaxAggregator<T>;
    MaxAggregator<T> aggregator() const { return {}; }
};

static_assert(Aggregate<MaxAggregate<int64_t>>);

template <typename T>
struct SumAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::stringstream, T>;

    SumAggregator() = default;
    SumAggregator(SumAggregator<T>&&) noexcept = default;

    void feed(std::span<Value> values) {
        verify_dbg(values.size() == 1);
        if (values[0] == vnull) {
            return;
        }

        if constexpr (std::same_as<U, T>) {
            sum += values[0].get<T>();
        } else {
            sum << values[0].get<std::string_view>();
        }
    }

    Value get() {
        if constexpr (std::same_as<U, T>) {
            return sum;
        } else {
            return sum.str();
        }
    }

    U sum = U();
};

static_assert(Aggregator<SumAggregator<int64_t>>);

template <Addable T>
struct SumAggregate {
    using Aggregator = SumAggregator<T>;
    SumAggregator<T> aggregator() const { return {}; }
};

static_assert(Aggregate<SumAggregate<int64_t>>);

template <typename T>
struct PercentileAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    explicit PercentileAggregator(const std::vector<float>& percentiles)
        : percentiles(percentiles) {}

    void feed(std::span<Value> args) {
        verify_dbg(args.size() == 1);

        if (args[0] != vnull) {
            values.emplace_back(args[0].get<T>());
        }
    }

    Value get() {
        if (values.empty()) {
            return vnull;
        }

        std::vector<U> result;
        result.reserve(percentiles.size());

        std::ptrdiff_t size = static_cast<std::ptrdiff_t>(values.size());
        auto left = values.begin();

        constexpr float threshold = 1e-6;

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

            auto it = std::next(values.begin(), pos);
            assert(left <= it);

            std::nth_element(left, it, values.end());
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

    const std::vector<float>& percentiles;
    std::vector<U> values;
};

static_assert(Aggregator<PercentileAggregator<int64_t>>);

template <Comparable T>
struct PercentileAggregate {
    using Aggregator = PercentileAggregator<T>;

    explicit PercentileAggregate(std::vector<float> percentiles)
        : percentiles(std::move(percentiles)) {}

    PercentileAggregator<T> aggregator() const { return PercentileAggregator<T>(percentiles); }
    std::vector<float> percentiles;
};

static_assert(Aggregate<PercentileAggregate<int64_t>>);

inline SubstrExecutor build(const Substr& s) {
    return {s.from, s.length};
}

inline CoalesceExecutor build(const Coalesce& /*s*/) {
    return {};
}

inline RSubstrExecutor build(const RSubstr& s) {
    return RSubstrExecutor(s.regex);
}

inline LikeExecutor build(const Like& s) {
    return LikeExecutor(s.regex);
}

inline CastExecutor build(const Cast& s) {
    return CastExecutor(s.cast_to);
}

inline CountNonNullAggregate build(const CountNonNull& /*s*/) {
    return {};
}

inline CountAllAggregate build(const CountAll& /*s*/) {
    return {};
}

template <Comparable T>
PercentileAggregate<T> build(const Percentile& s) {
    return PercentileAggregate<T>(s.percentiles);
}

template <Comparable T>
MinAggregate<T> build(const Min& /*s*/) {
    return {};
}

template <Comparable T>
MaxAggregate<T> build(const Max& /*s*/) {
    return {};
}

template <Addable T>
SumAggregate<T> build(const Sum& /*s*/) {
    return {};
}

template <typename R, typename F>
R buildScalar(const Function& func, F&& f) {
    return util::match(
        func,
        util::Overloaded{
            [&](const Substr& s) -> R { return f(build(s)); },
            [&](const Coalesce& s) -> R { return f(build(s)); },
            [&](const RSubstr& s) -> R { return f(build(s)); },
            [&](const Like& s) -> R { return f(build(s)); },
            [&](const Cast& s) -> R { return f(build(s)); },
            [](auto&&...) -> R { panic("not a supported scalar"); },
        });
}

template <typename R, typename F>
R buildAggregate(const Function& func, F&& f) {
    return util::match(
        func,
        util::Overloaded{
            [&](const CountNonNull& s) -> R { return f(build(s)); },
            [&](const CountAll& s) -> R { return f(build(s)); },
            [&](const Percentile& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.args_type);
            },
            [&](const Min& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Max& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Sum& s) -> R {
                return dispatch<R>(
                    [&]<Addable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [](auto&&...) -> R { panic("not a supported aggregate"); },
        });
}

}  // namespace lsql::func
