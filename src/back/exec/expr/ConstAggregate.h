#pragma once

#include "back/exec/expr/Aggregate.h"

namespace lsql::back::exec {

class ConstAggregate : public Aggregate {
    struct Aggr : Aggregator {
        Aggr(Value value, bool null_if_empty)
            : value_(std::move(value))
            , null_if_empty_(null_if_empty) {}

        void feed(const back::exec::Record& /*record*/) override { ++count_; }

        Value get() override {
            if (null_if_empty_ && count_ == 0) {
                return null;
            }

            return std::move(value_);
        }

        Value value_;
        bool null_if_empty_;
        int count_ = 0;
    };

 public:
    ConstAggregate(Value value, bool null_if_empty)
        : value_(std::move(value))
        , null_if_empty_(null_if_empty) {}

    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    ValueType valueType() const override { return value_.type(); }
    AggregatorPtr aggregator() const override { return arc<Aggr>(value_, null_if_empty_); }

 private:
    Value value_;
    bool null_if_empty_;
};

}  // namespace lsql::back::exec
