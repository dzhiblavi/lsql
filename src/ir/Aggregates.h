#pragma once

#include "ir/Aggregate.h"
#include "ir/Expr.h"

#include "core/expressions.h"
#include "core/Fields.h"
#include "core/types.h"

namespace lsql::ir {

struct ScalarAggregate {
    UnaryAggregateExprType type;
    Box<Expr> expr;
};

struct PercentileAggregate {
    Box<Expr> expr;
    std::vector<float> percentiles;
};

struct Aggregate {
    AggregateNode node;
    FieldId output_field_id;
    ValueType value_type;
};

}  // namespace lsql::ir
