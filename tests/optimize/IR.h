#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"
#include "ir/Statement.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/exprs/BinaryExpr.h"
#include "core/types.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace lsql::opt::test {

constexpr FieldId Timestamp = 1;
constexpr FieldId Output = 2;
constexpr FieldId Result = 3;

inline FieldSet fieldSet(std::initializer_list<FieldId> ids) {
    auto fields = FieldSet::emptySet();
    for (auto id : ids) {
        fields.add(id);
    }
    return fields;
}

inline ir::Scalar field(FieldId id, ValueType type = ValueType::String) {
    return ir::Scalar{
        .node = ir::FieldScalar{.field_id = id},
        .value_type = type,
    };
}

inline ir::Scalar boolean(bool value) {
    return ir::Scalar{
        .node = ir::ValueScalar{.value = Value(value)},
        .value_type = ValueType::Boolean,
    };
}

inline ir::Scalar integer(int64_t value) {
    return ir::Scalar{
        .node = ir::ValueScalar{.value = Value(value)},
        .value_type = ValueType::Integer,
    };
}

inline ir::Scalar null(ValueType value_type = ValueType::Null) {
    return ir::Scalar{
        .node = ir::ValueScalar{.value = Value(lsql::null)},
        .value_type = value_type,
    };
}

inline ir::Scalar coalesce(std::vector<ir::Scalar> args, ValueType value_type) {
    return ir::Scalar{
        .node = ir::CoalesceScalar{.args = std::move(args)},
        .value_type = value_type,
    };
}

inline ir::Scalar add(ir::Scalar left, ir::Scalar right) {
    return ir::Scalar{
        .node =
            ir::BinaryScalar{
                .type = BinaryExprType::Add,
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .value_type = ValueType::Integer,
    };
}

inline ir::Relation empty(FieldSet fields = FieldSet::emptySet()) {
    return ir::Relation{
        .node = ir::EmptyRelation{},
        .fields_out = fields,
    };
}

inline ir::Relation file(
    std::string path = "app.log", FieldSet fields = FieldSet::withField(Timestamp)) {
    return ir::Relation{
        .node = ir::FileRelation{.path = std::move(path)},
        .fields_out = fields,
    };
}

inline ir::Relation filter(ir::Relation source, ir::Scalar condition) {
    auto fields = source.fields_out;
    return ir::Relation{
        .node =
            ir::FilterRelation{
                .source = box(std::move(source)),
                .condition = box(std::move(condition)),
            },
        .fields_out = fields,
    };
}

inline ir::Relation sort(ir::Relation source, bool desc = true) {
    auto fields = source.fields_out;
    std::vector<ir::Scalar> order_list;
    order_list.push_back(field(Timestamp));

    return ir::Relation{
        .node =
            ir::SortRelation{
                .source = box(std::move(source)),
                .order_list = std::move(order_list),
                .desc = desc,
            },
        .fields_out = fields,
    };
}

inline ir::Relation limit(ir::Relation source, int count) {
    auto fields = source.fields_out;
    return ir::Relation{
        .node =
            ir::LimitRelation{
                .source = box(std::move(source)),
                .limit = count,
            },
        .fields_out = fields,
    };
}

inline ir::Projector projector(FieldId id, ir::Scalar expr) {
    return ir::Projector{
        .alias_field_id = id,
        .expr = box(std::move(expr)),
    };
}

inline ir::Relation project(
    ir::Relation source, std::vector<ir::Projector> projectors, FieldSet fields) {
    return ir::Relation{
        .node =
            ir::ProjectionRelation{
                .source = box(std::move(source)),
                .projectors = std::move(projectors),
            },
        .fields_out = fields,
    };
}

inline ir::Relation project(ir::Relation source, ir::Projector projector, FieldSet fields) {
    std::vector<ir::Projector> projectors;
    projectors.push_back(std::move(projector));
    return project(std::move(source), std::move(projectors), fields);
}

inline ir::Aggregate min(FieldId id, ir::Scalar expr) {
    return ir::Aggregate{
        .node =
            ir::UnaryAggregate{
                .type = UnaryAggregateType::Min,
                .expr = box(std::move(expr)),
            },
        .output_field_id = id,
        .value_type = ValueType::Integer,
    };
}

inline ir::Aggregate constant(FieldId id, Value value, bool null_if_empty) {
    auto value_type = value.type();
    return ir::Aggregate{
        .node =
            ir::ConstAggregate{
                .value = std::move(value),
                .null_if_empty = null_if_empty,
            },
        .output_field_id = id,
        .value_type = value_type,
    };
}

inline ir::Relation aggregate(ir::Relation source, ir::Aggregate aggregate, FieldSet fields) {
    std::vector<ir::Aggregate> aggregates;
    aggregates.push_back(std::move(aggregate));

    return ir::Relation{
        .node =
            ir::AggregateRelation{
                .source = box(std::move(source)),
                .aggregates = std::move(aggregates),
            },
        .fields_out = fields,
    };
}

inline std::vector<ir::Scalar> timestampOrderList() {
    std::vector<ir::Scalar> order_list;
    order_list.push_back(field(Timestamp));
    return order_list;
}

inline ir::Relation topK(ir::Relation source, int count, bool desc = true) {
    auto fields = source.fields_out;
    return ir::Relation{
        .node =
            ir::TopKRelation{
                .source = box(std::move(source)),
                .order_list = timestampOrderList(),
                .desc = desc,
                .top_count = count,
            },
        .fields_out = fields,
    };
}

inline ir::Program query(ir::Relation relation) {
    std::vector<ir::Statement> statements;
    statements.push_back(ir::QueryStatement{.relation = box(std::move(relation))});

    return ir::Program{
        .statements = std::move(statements),
        .field_binding = {},
    };
}

}  // namespace lsql::opt::test
