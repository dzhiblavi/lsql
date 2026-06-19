#include "core/function/build.h"
#include "core/exprs/concepts.h"
#include "core/value/cast.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>
#include <reflex/stdmatcher.h>

#include <algorithm>

namespace lsql::func {

namespace {

Arc<Executor> buildScalar(const Substr& substr) {
    struct Executor : func::Executor {
        Executor(size_t from, size_t len) : from(from), length(len) {}

        Value execute(std::span<Value> values) const override {
            verify_dbg(values.size() == 1);
            return values[0].substr(from, length);
        }

        size_t from;
        size_t length;
    };

    return arc<Executor>(substr.from, substr.length);
}

Arc<Executor> buildScalar(const Coalesce&) {
    struct Executor : func::Executor {
        Value execute(std::span<Value> values) const override {
            for (auto&& value : values) {
                if (value != vnull) {
                    return value;
                }
            }
            return vnull;
        }
    };

    return arc<Executor>();
}

Arc<Executor> buildScalar(const RSubstr& r) {
    struct Executor : func::Executor {
        explicit Executor(const std::string& regex) : pattern(regex) {}

        Value execute(std::span<Value> values) const override {
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

        const reflex::Pattern pattern;
    };

    return arc<Executor>(r.regex);
}

Arc<Executor> buildScalar(const Like& r) {
    struct Executor : func::Executor {
        explicit Executor(const std::string& regex) : pattern(regex) {}

        Value execute(std::span<Value> values) const override {
            verify_dbg(values.size() == 1);
            auto view = values[0].get<std::string_view>();
            auto input = reflex::Input(view.data(), view.size());
            return reflex::Matcher(&pattern, input).matches() != 0;
        }

        const reflex::Pattern pattern;
    };

    return arc<Executor>(r.regex);
}

Arc<Executor> buildScalar(const Cast& r) {
    struct Executor : func::Executor {
        explicit Executor(ValueType cast_to) : cast_to(cast_to) {}

        Value execute(std::span<Value> values) const override {
            verify_dbg(values.size() == 1);
            return cast(std::move(values[0]), cast_to).value_or(null);
        }

        ValueType cast_to;
    };

    return arc<Executor>(r.cast_to);
}

template <typename T>
struct PercentileAggregator : Aggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    explicit PercentileAggregator(const std::vector<float>& percentiles)
        : percentiles(percentiles) {}

    void feed(std::span<Value> args) override {
        verify_dbg(args.size() == 1);

        if (args[0] != vnull) {
            values.emplace_back(args[0].get<T>());
        }
    }

    Value get() override {
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

template <typename T>
struct PercentileAggregate : Aggregate {
    explicit PercentileAggregate(std::vector<float> percentiles)
        : percentiles(std::move(percentiles)) {}

    Box<Aggregator> aggregator() const override {
        return box<PercentileAggregator<T>>(percentiles);
    }

    std::vector<float> percentiles;
};

template <typename T>
struct SumAggregator : func::Aggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::stringstream, T>;

    void feed(std::span<Value> values) override {
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

    Value get() override {
        if constexpr (std::same_as<U, T>) {
            return sum;
        } else {
            return sum.str();
        }
    }

    U sum = U();
};

template <typename T>
struct SumAggregate : func::Aggregate {
    Box<Aggregator> aggregator() const override { return box<SumAggregator<T>>(); }
};

template <typename T>
struct MinAggregator : func::Aggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(std::span<Value> values) override {
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

    Value get() override {
        if (res.has_value()) {
            return std::move(*res);
        }
        return vnull;
    }

    std::optional<U> res = std::nullopt;
};

template <typename T>
struct MinAggregate : func::Aggregate {
    Box<Aggregator> aggregator() const override { return box<MinAggregator<T>>(); }
};

template <typename T>
struct MaxAggregator : func::Aggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(std::span<Value> values) override {
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

    Value get() override {
        if (res.has_value()) {
            return std::move(*res);
        }
        return vnull;
    }

    std::optional<U> res = std::nullopt;
};

template <typename T>
struct MaxAggregate : func::Aggregate {
    Box<Aggregator> aggregator() const override { return box<MaxAggregator<T>>(); }
};

Arc<Aggregate> buildAggregate(const Percentile& p) {
    return dispatch<Arc<Aggregate>>(
        [&]<Comparable T>(std::type_identity<T>) -> Arc<Aggregate> {
            return arc<PercentileAggregate<T>>(std::move(p.percentiles));
        },
        p.args_type);
}

Arc<Aggregate> buildAggregate(const CountNonNull&) {
    struct Aggregator : func::Aggregator {
        void feed(std::span<Value> values) override {
            verify_dbg(values.size() == 1);
            count += (values[0] != vnull);
        }

        Value get() override { return count; }

        int64_t count = 0;
    };
    struct Aggregate : func::Aggregate {
        Box<func::Aggregator> aggregator() const override { return box<Aggregator>(); }
    };
    return arc<Aggregate>();
}

Arc<Aggregate> buildAggregate(const CountAll&) {
    struct Aggregator : func::Aggregator {
        void feed(std::span<Value> /*values*/) override { ++count; }
        Value get() override { return count; }
        int64_t count = 0;
    };
    struct Aggregate : func::Aggregate {
        Box<func::Aggregator> aggregator() const override { return box<Aggregator>(); }
    };
    return arc<Aggregate>();
}

Arc<Aggregate> buildAggregate(const Sum& p) {
    return dispatch<Arc<Aggregate>>(
        [&]<Addable T>(std::type_identity<T>) -> Arc<Aggregate> { return arc<SumAggregate<T>>(); },
        p.arg_type);
}

Arc<Aggregate> buildAggregate(const Min& p) {
    return dispatch<Arc<Aggregate>>(
        [&]<Comparable T>(std::type_identity<T>) -> Arc<Aggregate> {
            return arc<MinAggregate<T>>();
        },
        p.arg_type);
}

Arc<Aggregate> buildAggregate(const Max& p) {
    return dispatch<Arc<Aggregate>>(
        [&]<Comparable T>(std::type_identity<T>) -> Arc<Aggregate> {
            return arc<MaxAggregate<T>>();
        },
        p.arg_type);
}

}  // namespace

Arc<Executor> buildScalar(const Function& func) {
    verify(isScalar(func));

    return util::match(func, [](auto&& variant) {
        if constexpr (requires { buildScalar(variant); }) {
            return buildScalar(variant);
        } else {
            panic("not implemented");
        }
    });
}

Arc<Aggregate> buildAggregate(const Function& func) {
    verify(!isScalar(func));

    return util::match(func, [](auto&& variant) {
        if constexpr (requires { buildAggregate(variant); }) {
            return buildAggregate(variant);
        } else {
            panic("not implemented");
        }
    });
}

}  // namespace lsql::func
