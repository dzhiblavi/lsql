#pragma once

#include "back/exec/expr/Aggregate.h"

#include "core/function/Aggregate.h"

namespace lsql::back::exec {

template <func::NullaryAggregate A>
class NullaryFnAggregate : public exec::Aggregate {
    using FnAggregator = typename A::Aggregator;

    struct Aggregator : exec::Aggregator {
        explicit Aggregator(FnAggregator aggregator) : aggregator(std::move(aggregator)) {}
        void feed(const back::exec::Record& /*record*/) override { aggregator.feed(); }
        Value get() override { return std::move(aggregator).get(); }
        [[no_unique_address]] FnAggregator aggregator;
    };

 public:
    NullaryFnAggregate(ValueType value_type, A aggregate)
        : value_type_(value_type)
        , aggregate_(std::move(aggregate)) {}

    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    ValueType valueType() const override { return value_type_; }

    AggregatorPtr aggregator() const override { return arc<Aggregator>(aggregate_.aggregator()); }

 private:
    ValueType value_type_;
    [[no_unique_address]] A aggregate_;
};

}  // namespace lsql::back::exec
