#pragma once

#include "core/Value.h"
#include "ir/Aggregate.h"
#include "ir/Scalar.h"

#include "core/Fields.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/types.h"

namespace lsql::ir {

struct UnaryAggregate {
    UnaryAggregateType type;
    Box<Scalar> expr;
};

struct CountAllAggregate {};

struct PercentileAggregate {
    Box<Scalar> expr;
    std::vector<float> percentiles;
};

struct ConstAggregate {
    Value value;
    bool null_if_empty;
};

struct Aggregate {
    AggregateNode node;
    FieldId output_field_id;
    ValueType value_type;
};

}  // namespace lsql::ir
