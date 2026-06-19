#pragma once

#include "back/exec/expr/Aggregate.h"

#include "back/exec/expr/Scalar.h"

#include "core/function/Aggregate.h"

namespace lsql::back::exec {

template <func::UnaryAggregate A>
class UnaryFnAggregate : public exec::Aggregate {
    using FnAggregator = typename A::Aggregator;

    struct Aggregator : exec::Aggregator {
        Aggregator(const Scalar* arg, FnAggregator aggregator)
            : arg(arg)
            , aggregator(std::move(aggregator)) {}

        void feed(const back::exec::Record& record) override { aggregator.feed(arg->eval(record)); }
        Value get() override { return std::move(aggregator).get(); }

        const Scalar* arg;
        [[no_unique_address]] FnAggregator aggregator;
    };

 public:
    UnaryFnAggregate(ValueType value_type, A aggregate, Arc<Scalar> arg)
        : value_type_(value_type)
        , arg_(std::move(arg))
        , aggregate_(std::move(aggregate)) {}

    FieldSet requiredFields() const override { return arg_->requiredFields(); }
    ValueType valueType() const override { return value_type_; }

    AggregatorPtr aggregator() const override {
        return arc<Aggregator>(arg_.get(), aggregate_.aggregator());
    }

 private:
    ValueType value_type_;
    Arc<Scalar> arg_;
    [[no_unique_address]] A aggregate_;
};

}  // namespace lsql::back::exec
