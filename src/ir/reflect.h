#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"

#include <tuple>

namespace lsql::ir {

template <typename IRNode>
struct Reflect;

template <>
struct Reflect<FieldScalar> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(&FieldScalar::field_id); }
};

template <>
struct Reflect<ValueScalar> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(&ValueScalar::value); }
};

template <>
struct Reflect<CoalesceScalar> {
    static auto childNodes() { return std::make_tuple(&CoalesceScalar::args); }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<FnCallScalar> {
    static auto childNodes() { return std::make_tuple(&FnCallScalar::args); }
    static auto fields() { return std::make_tuple(&FnCallScalar::function); }
};

template <>
struct Reflect<Scalar> {
    static auto childNodes() { return std::make_tuple(&Scalar::node); }
    static auto fields() { return std::make_tuple(&Scalar::value_type); }
};

template <>
struct Reflect<FnCallAggregate> {
    static auto childNodes() { return std::make_tuple(&FnCallAggregate::args); }
    static auto fields() { return std::make_tuple(&FnCallAggregate::function); }
};

template <>
struct Reflect<ConstAggregate> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() {
        return std::make_tuple(&ConstAggregate::value, &ConstAggregate::null_if_empty);
    }
};

template <>
struct Reflect<Aggregate> {
    static auto childNodes() { return std::make_tuple(&Aggregate::node); }
    static auto fields() {
        return std::make_tuple(&Aggregate::output_field_id, &Aggregate::value_type);
    }
};

template <>
struct Reflect<NamedRelationStatement> {
    static auto childNodes() { return std::make_tuple(&NamedRelationStatement::relation); }
    static auto fields() { return std::make_tuple(&NamedRelationStatement::name); }
};

template <>
struct Reflect<QueryStatement> {
    static auto childNodes() { return std::make_tuple(&QueryStatement::relation); }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<Projector> {
    static auto childNodes() { return std::make_tuple(&Projector::expr); }
    static auto fields() { return std::make_tuple(&Projector::alias_field_id); }
};

template <>
struct Reflect<EmptyRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<ValuesRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() {
        return std::make_tuple(&ValuesRelation::output_id, &ValuesRelation::values);
    }
};

template <>
struct Reflect<ProjectionRelation> {
    static auto childNodes() {
        return std::make_tuple(&ProjectionRelation::source, &ProjectionRelation::projectors);
    }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<AggregateRelation> {
    static auto childNodes() {
        return std::make_tuple(&AggregateRelation::source, &AggregateRelation::aggregates);
    }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<GroupRelation> {
    static auto childNodes() {
        return std::make_tuple(
            &GroupRelation::source, &GroupRelation::aggregates, &GroupRelation::group_list);
    }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<LimitRelation> {
    static auto childNodes() { return std::make_tuple(&LimitRelation::source); }
    static auto fields() { return std::make_tuple(&LimitRelation::limit); }
};

template <>
struct Reflect<FilterRelation> {
    static auto childNodes() {
        return std::make_tuple(&FilterRelation::source, &FilterRelation::condition);
    }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<SortRelation> {
    static auto childNodes() {
        return std::make_tuple(&SortRelation::source, &SortRelation::order_list);
    }
    static auto fields() { return std::make_tuple(&SortRelation::desc); }
};

template <>
struct Reflect<TopKRelation> {
    static auto childNodes() {
        return std::make_tuple(&TopKRelation::source, &TopKRelation::order_list);
    }
    static auto fields() { return std::make_tuple(&TopKRelation::desc, &TopKRelation::top_count); }
};

template <>
struct Reflect<SemiJoinRelation> {
    static auto childNodes() {
        return std::make_tuple(
            &SemiJoinRelation::source, &SemiJoinRelation::match, &SemiJoinRelation::expr);
    }
    static auto fields() { return std::make_tuple(&SemiJoinRelation::match_field_id); }
};

template <>
struct Reflect<MarkJoinRelation> {
    static auto childNodes() {
        return std::make_tuple(
            &MarkJoinRelation::source, &MarkJoinRelation::match, &MarkJoinRelation::expr);
    }
    static auto fields() {
        return std::make_tuple(
            &MarkJoinRelation::match_field_id, &MarkJoinRelation::output_field_id);
    }
};

template <>
struct Reflect<UnionAllRelation> {
    static auto childNodes() {
        return std::make_tuple(&UnionAllRelation::left, &UnionAllRelation::right);
    }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<UnionAllSortedByRelation> {
    static auto childNodes() {
        return std::make_tuple(
            &UnionAllSortedByRelation::left,
            &UnionAllSortedByRelation::right,
            &UnionAllSortedByRelation::order_list);
    }
    static auto fields() { return std::make_tuple(&UnionAllSortedByRelation::desc); }
};

template <>
struct Reflect<FileRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(&FileRelation::path); }
};

template <>
struct Reflect<FileIntervalRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() {
        return std::make_tuple(
            &FileIntervalRelation::path,
            &FileIntervalRelation::ts_from,
            &FileIntervalRelation::ts_to);
    }
};

template <>
struct Reflect<StreamRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(&StreamRelation::command); }
};

template <>
struct Reflect<NamedRelationReferenceRelation> {
    static auto childNodes() { return std::make_tuple(); }
    static auto fields() { return std::make_tuple(&NamedRelationReferenceRelation::name); }
};

template <>
struct Reflect<MaterializeRelation> {
    static auto childNodes() { return std::make_tuple(&MaterializeRelation::source); }
    static auto fields() { return std::make_tuple(); }
};

template <>
struct Reflect<Relation> {
    static auto childNodes() { return std::make_tuple(&Relation::node); }
    static auto fields() { return std::make_tuple(&Relation::schema); }
};

template <typename N>
concept Reflectable = requires {
    { Reflect<N>::childNodes() };
    { Reflect<N>::fields() };
};

template <typename Node>
auto childNodes(const Node& node) {
    using Reflection = Reflect<Node>;

    return std::apply(
        [&node](auto... field_ptr) { return std::tie(node.*field_ptr...); },
        Reflection::childNodes());
}

template <typename Node>
auto fields(const Node& node) {
    using Reflection = Reflect<Node>;

    return std::apply(
        [&node](auto... field_ptr) { return std::tie(node.*field_ptr...); }, Reflection::fields());
}

}  // namespace lsql::ir
