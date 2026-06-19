#pragma once

#include "back/exec/expr/Aggregate.h"

#include "back/exec/expr/Scalar.h"

#include "core/function/Aggregate.h"

#include <ranges>

namespace lsql::back::exec {

template <func::Aggregate A>
class FnAggregate : public exec::Aggregate {
    using FnAggregator = typename A::Aggregator;

    struct Aggregator : exec::Aggregator {
        Aggregator(const std::vector<Arc<Scalar>>* args, FnAggregator aggregator)
            : args(args)
            , aggregator(std::move(aggregator)) {
            values.assign(args->size(), vnull);
        }

        void feed(const back::exec::Record& record) override {
            for (auto&& [arg, value] : std::views::zip(*args, values)) {
                value = arg->eval(record);
            }
            aggregator.feed(values);
        }

        Value get() override { return std::move(aggregator).get(); }

        const std::vector<Arc<Scalar>>* args;
        mutable std::vector<Value> values;
        [[no_unique_address]] FnAggregator aggregator;
    };

 public:
    FnAggregate(ValueType value_type, A aggregate, std::vector<Arc<Scalar>> args)
        : value_type_(value_type)
        , args_(std::move(args))
        , aggregate_(std::move(aggregate)) {}

    FieldSet requiredFields() const override {
        FieldSet set;
        for (auto&& arg : args_) {
            set.merge(arg->requiredFields());
        }
        return set;
    }

    ValueType valueType() const override { return value_type_; }

    AggregatorPtr aggregator() const override {
        return arc<Aggregator>(&args_, aggregate_.aggregator());
    }

 private:
    ValueType value_type_;
    std::vector<Arc<Scalar>> args_;
    [[no_unique_address]] A aggregate_;
};

}  // namespace lsql::back::exec
