#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Function.h"

#include <algorithm>

namespace lsql::func {

template <typename T>
struct PercentileAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    explicit PercentileAggregator(const std::vector<float>& percentiles)
        : percentiles(percentiles) {}

    void feed(const Value& value) {
        if (value != vnull) {
            values.emplace_back(value.get<T>());
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

static_assert(UnaryAggregator<PercentileAggregator<int64_t>>);

template <Comparable T>
struct PercentileAggregate {
    using Aggregator = PercentileAggregator<T>;

    explicit PercentileAggregate(std::vector<float> percentiles)
        : percentiles(std::move(percentiles)) {}

    PercentileAggregator<T> aggregator() const { return PercentileAggregator<T>(percentiles); }
    std::vector<float> percentiles;
};

static_assert(UnaryAggregate<PercentileAggregate<int64_t>>);

template <Comparable T>
PercentileAggregate<T> build(const Percentile& s) {
    return PercentileAggregate<T>(s.percentiles);
}

}  // namespace lsql::func
