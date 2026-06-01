#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"

namespace lsql::opt {

template <typename F>
void constPass(const ir::Relation& rel, F& f);

template <typename F>
void constPass(const ir::Statement& st, F& f);

template <typename F>
void constPass(const ir::Scalar& sc, F& f);

template <typename F>
void constPass(const ir::Aggregate& ag, F& f);

template <typename F>
void constPass(const std::vector<ir::Projector>& ps, F& f) {
    for (auto&& p : ps) {
        constPass(p, f);
    }
}

template <typename F>
void constPass(const std::vector<ir::Scalar>& ps, F& f) {
    for (auto&& p : ps) {
        constPass(p, f);
    }
}

template <typename F>
void constPass(const std::vector<ir::Aggregate>& ps, F& f) {
    for (auto&& p : ps) {
        constPass(p, f);
    }
}

template <typename N, typename T, typename F>
void callConst(N& node, T& entity, F& f) {
    f(node, entity);
}

template <typename F>
void constPass(const ir::EmptyRelation& rel, ir::Relation& self, F& f) {
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::ValuesRelation& rel, ir::Relation& self, F& f) {
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::ProjectionRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(rel.projectors, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::AggregateRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(rel.aggregates, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::GroupRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(rel.aggregates, f);
    constPass(rel.group_list, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::LimitRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::FilterRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(*rel.condition, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::SortRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(rel.order_list, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::TopKRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(rel.order_list, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::SemiJoinRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(*rel.match, f);
    constPass(*rel.expr, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::MarkJoinRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.source, f);
    constPass(*rel.match, f);
    constPass(*rel.expr, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::UnionAllRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.left, f);
    constPass(*rel.right, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::UnionAllSortedByRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.left, f);
    constPass(*rel.right, f);
    constPass(rel.order_list, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::FileRelation& rel, ir::Relation& self, F& f) {
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::FileIntervalRelation& rel, ir::Relation& self, F& f) {
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::NamedRelationReferenceRelation& rel, ir::Relation& self, F& f) {
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::MaterializeRelation& rel, ir::Relation& self, F& f) {
    constPass(*rel.relation, f);
    callConst(rel, self, f);
}

template <typename F>
void constPass(const ir::NamedRelationStatement& st, auto& self, F& f) {
    constPass(*st.relation, f);
    callConst(st, self, f);
}

template <typename F>
void constPass(const ir::QueryStatement& st, auto& self, F& f) {
    constPass(*st.relation, f);
    callConst(st, self, f);
}

template <typename F>
void constPass(const ir::ValueScalar& s, auto& self, F& f) {
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::FieldScalar& s, auto& self, F& f) {
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::CoalesceScalar& s, auto& self, F& f) {
    constPass(s.args, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::CastScalar& s, auto& self, F& f) {
    constPass(*s.expr, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::LikeScalar& s, auto& self, F& f) {
    constPass(*s.expr, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::RSubstrScalar& s, auto& self, F& f) {
    constPass(*s.expr, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::UnaryScalar& s, auto& self, F& f) {
    constPass(*s.expr, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::BinaryScalar& s, auto& self, F& f) {
    constPass(*s.left, f);
    constPass(*s.right, f);
    callConst(s, self, f);
}

template <typename F>
void constPass(const ir::UnaryAggregate& a, auto& self, F& f) {
    constPass(*a.expr, f);
    callConst(a, self, f);
}

template <typename F>
void constPass(const ir::PercentileAggregate& a, auto& self, F& f) {
    constPass(*a.expr, f);
    callConst(a, self, f);
}

template <typename F>
void constPass(const ir::ConstAggregate& a, auto& self, F& f) {
    callConst(a, self, f);
}

template <typename F>
void constPass(const ir::Relation& rel, F& f) {
    util::match(rel.node, [&](auto& r) { constPass(r, rel, f); });
}

template <typename F>
void constPass(const ir::Statement& st, F& f) {
    util::match(st, [&](auto& s) { constPass(s, st, f); });
}

template <typename F>
void constPass(const ir::Scalar& sc, F& f) {
    util::match(sc.node, [&](auto& s) { constPass(s, sc, f); });
}

template <typename F>
void constPass(const ir::Aggregate& ag, F& f) {
    util::match(ag.node, [&](auto& s) { constPass(s, ag, f); });
}

}  // namespace lsql::opt
