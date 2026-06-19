#pragma once

#include "back/exec/expr/Aggregate.h"

#include "back/exec/expr/Scalar.h"
#include "core/function/Aggregate.h"

#include <ranges>

namespace lsql::back::exec {

class FnCallAggregate : public Aggregate {
    struct Aggregator : exec::Aggregator {
        Aggregator(const std::vector<Arc<Scalar>>* args, Arc<func::Aggregator> aggregator)
            : args(args)
            , aggregator(aggregator) {
            values.assign(args->size(), vnull);
        }

        void feed(const back::exec::Record& record) override {
            for (auto&& [arg, value] : std::views::zip(*args, values)) {
                value = arg->eval(record);
            }
            aggregator->feed(values);
        }

        Value get() override { return aggregator->get(); }

        const std::vector<Arc<Scalar>>* args;
        Arc<func::Aggregator> aggregator;
        mutable std::vector<Value> values;
    };

 public:
    FnCallAggregate(
        ValueType value_type, Arc<func::Aggregate> aggregate, std::vector<Arc<Scalar>> args)
        : value_type_(value_type)
        , aggregate_(std::move(aggregate))
        , args_(std::move(args)) {}

    FieldSet requiredFields() const override {
        FieldSet set;
        for (auto&& arg : args_) {
            set.merge(arg->requiredFields());
        }
        return set;
    }

    ValueType valueType() const override { return value_type_; }

    AggregatorPtr aggregator() const override {
        return arc<Aggregator>(&args_, aggregate_->aggregator());
    }

 private:
    ValueType value_type_;
    Arc<func::Aggregate> aggregate_;
    std::vector<Arc<Scalar>> args_;
};

}  // namespace lsql::back::exec
