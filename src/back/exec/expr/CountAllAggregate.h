#pragma once

#include "back/exec/expr/Aggregate.h"

#include <llog/log.h>

namespace lsql::back::exec {

class CountAllAggregate : public Aggregate {
    struct Aggr : Aggregator {
        void feed(const back::exec::Record& /*record*/) override { ++count; }
        Value get() override { return count; }
        int64_t count = 0;
    };

 public:
    CountAllAggregate() = default;
    FieldSet requiredFields() const override { return FieldSet::emptySet(); }
    ValueType valueType() const override { return ValueType::Integer; }
    AggregatorPtr aggregator() const override { return arc<Aggr>(); }
};

}  // namespace lsql::back::exec
