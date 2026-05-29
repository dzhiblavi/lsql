#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"

namespace lsql::opt {

template <typename F>
ir::Relation pass(ir::Relation rel, F& f);

template <typename F>
ir::Statement pass(ir::Statement st, F& f);

template <typename F>
ir::Scalar pass(ir::Scalar sc, F& f);

template <typename F>
ir::Aggregate pass(ir::Aggregate ag, F& f);

template <typename F>
std::vector<ir::Projector> pass(std::vector<ir::Projector> ps, F& f) {
    std::vector<ir::Projector> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back({
            .alias_field_id = p.alias_field_id,
            .expr = box(pass(std::move(*p.expr), f)),
        });
    }
    return r;
}

template <typename F>
std::vector<ir::Scalar> pass(std::vector<ir::Scalar> ps, F& f) {
    std::vector<ir::Scalar> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back(pass(std::move(p), f));
    }
    return r;
}

template <typename F>
std::vector<ir::Aggregate> pass(std::vector<ir::Aggregate> ps, F& f) {
    std::vector<ir::Aggregate> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back(pass(std::move(p), f));
    }
    return r;
}

template <typename N, typename T, typename F>
T call(N& node, T& entity, F& f) {
    return f(node, entity);
}

template <typename F>
ir::Relation pass(ir::ValuesRelation& rel, ir::Relation& self, F& f) {
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::ProjectionRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.projectors = pass(std::move(rel.projectors), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::AggregateRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.aggregates = pass(std::move(rel.aggregates), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::GroupRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.aggregates = pass(std::move(rel.aggregates), f);
    rel.group_list = pass(std::move(rel.group_list), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::LimitRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::FilterRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.condition = box(pass(std::move(*rel.condition), f));
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::SortRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.order_list = pass(std::move(rel.order_list), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::TopKRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.order_list = pass(std::move(rel.order_list), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::SemiJoinRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.match = box(pass(std::move(*rel.match), f));
    rel.expr = box(pass(std::move(*rel.expr), f));
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::MarkJoinRelation& rel, ir::Relation& self, F& f) {
    rel.source = box(pass(std::move(*rel.source), f));
    rel.match = box(pass(std::move(*rel.match), f));
    rel.expr = box(pass(std::move(*rel.expr), f));
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::UnionAllRelation& rel, ir::Relation& self, F& f) {
    rel.left = box(pass(std::move(*rel.left), f));
    rel.right = box(pass(std::move(*rel.right), f));
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::UnionAllSortedByRelation& rel, ir::Relation& self, F& f) {
    rel.left = box(pass(std::move(*rel.left), f));
    rel.right = box(pass(std::move(*rel.right), f));
    rel.order_list = pass(std::move(rel.order_list), f);
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::FileRelation& rel, ir::Relation& self, F& f) {
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::FileIntervalRelation& rel, ir::Relation& self, F& f) {
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::NamedRelationReferenceRelation& rel, ir::Relation& self, F& f) {
    return call(rel, self, f);
}

template <typename F>
ir::Relation pass(ir::MaterializeRelation& rel, ir::Relation& self, F& f) {
    rel.relation = box(pass(std::move(*rel.relation), f));
    return call(rel, self, f);
}

template <typename F>
ir::Statement pass(ir::NamedRelationStatement& st, auto& self, F& f) {
    st.relation = box(pass(std::move(*st.relation), f));
    return call(st, self, f);
}

template <typename F>
ir::Statement pass(ir::QueryStatement& st, auto& self, F& f) {
    st.relation = box(pass(std::move(*st.relation), f));
    return call(st, self, f);
}

template <typename F>
ir::Scalar pass(ir::ValueScalar& s, auto& self, F& f) {
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::FieldScalar& s, auto& self, F& f) {
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::CoalesceScalar& s, auto& self, F& f) {
    s.args = pass(std::move(s.args), f);
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::CastScalar& s, auto& self, F& f) {
    s.expr = box(pass(std::move(*s.expr), f));
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::LikeScalar& s, auto& self, F& f) {
    s.expr = box(pass(std::move(*s.expr), f));
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::RSubstrScalar& s, auto& self, F& f) {
    s.expr = box(pass(std::move(*s.expr), f));
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::UnaryScalar& s, auto& self, F& f) {
    s.expr = box(pass(std::move(*s.expr), f));
    return call(s, self, f);
}

template <typename F>
ir::Scalar pass(ir::BinaryScalar& s, auto& self, F& f) {
    s.left = box(pass(std::move(*s.left), f));
    s.right = box(pass(std::move(*s.right), f));
    return call(s, self, f);
}

template <typename F>
ir::Aggregate pass(ir::UnaryAggregate& a, auto& self, F& f) {
    a.expr = box(pass(std::move(*a.expr), f));
    return call(a, self, f);
}

template <typename F>
ir::Aggregate pass(ir::PercentileAggregate& a, auto& self, F& f) {
    a.expr = box(pass(std::move(*a.expr), f));
    return call(a, self, f);
}

template <typename F>
ir::Aggregate pass(ir::ConstAggregate& a, auto& self, F& f) {
    return call(a, self, f);
}

template <typename F>
ir::Relation pass(ir::Relation rel, F& f) {
    return util::match(rel.node, [&](auto& r) -> ir::Relation { return pass(r, rel, f); });
}

template <typename F>
ir::Statement pass(ir::Statement st, F& f) {
    return util::match(st, [&](auto& s) -> ir::Statement { return pass(s, st, f); });
}

template <typename F>
ir::Scalar pass(ir::Scalar sc, F& f) {
    return util::match(sc.node, [&](auto& s) -> ir::Scalar { return pass(s, sc, f); });
}

template <typename F>
ir::Aggregate pass(ir::Aggregate ag, F& f) {
    return util::match(ag.node, [&](auto& s) -> ir::Aggregate { return pass(s, ag, f); });
}

}  // namespace lsql::opt
