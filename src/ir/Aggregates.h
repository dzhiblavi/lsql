#pragma once

#include "ir/Aggregate.h"
#include "ir/Scalar.h"

#include "core/function/Function.h"
#include "core/schema/types.h"
#include "core/value/Value.h"

#include <cstddef>
#include <functional>

namespace lsql::ir {

struct FnCallAggregate {
    func::Function function;
    std::vector<Scalar> args;
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

bool operator==(const Aggregate& left, const Aggregate& right);

}  // namespace lsql::ir

template <>
struct std::hash<lsql::ir::Aggregate> {
    size_t operator()(const lsql::ir::Aggregate& aggregate) const;
};
