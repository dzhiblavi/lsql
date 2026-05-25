#pragma once

#include "ir/Aggregate.h"
#include "ir/Scalar.h"

#include "core/expressions.h"
#include "core/Fields.h"
#include "core/types.h"

namespace lsql::ir {

struct UnaryAggregate {
    UnaryAggregateExprType type;
    Box<Scalar> expr;
};

struct PercentileAggregate {
    Box<Scalar> expr;
    std::vector<float> percentiles;
};

struct Aggregate {
    AggregateNode node;
    FieldId output_field_id;
    ValueType value_type;
};

}  // namespace lsql::ir
