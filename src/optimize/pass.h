#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"

namespace lsql::opt {

class IRTree {
 public:
    template <typename>
    struct FieldsOf {};

    template <>
    struct FieldsOf<ir::FieldScalar> {
        static auto get() { return std::tuple<>(); }
    };

    template <>
    struct FieldsOf<ir::ValueScalar> {
        static auto get() { return std::tuple<>(); }
    };

    template <>
    struct FieldsOf<ir::CoalesceScalar> {
        static auto get() { return std::make_tuple(&ir::CoalesceScalar::args); }
    };

    template <>
    struct FieldsOf<ir::CastScalar> {
        static auto get() { return std::make_tuple(&ir::CastScalar::expr); }
    };

    template <>
    struct FieldsOf<ir::LikeScalar> {
        static auto get() { return std::make_tuple(&ir::LikeScalar::expr); }
    };

    template <>
    struct FieldsOf<ir::RSubstrScalar> {
        static auto get() { return std::make_tuple(&ir::RSubstrScalar::expr); }
    };

    template <>
    struct FieldsOf<ir::UnaryScalar> {
        static auto get() { return std::make_tuple(&ir::UnaryScalar::expr); }
    };

    template <>
    struct FieldsOf<ir::BinaryScalar> {
        static auto get() {
            return std::make_tuple(&ir::BinaryScalar::left, &ir::BinaryScalar::right);
        }
    };

    template <>
    struct FieldsOf<ir::UnaryAggregate> {
        static auto get() { return std::make_tuple(&ir::UnaryAggregate::expr); }
    };

    template <>
    struct FieldsOf<ir::CountAllAggregate> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::PercentileAggregate> {
        static auto get() { return std::make_tuple(&ir::PercentileAggregate::expr); }
    };

    template <>
    struct FieldsOf<ir::ConstAggregate> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::NamedRelationStatement> {
        static auto get() { return std::make_tuple(&ir::NamedRelationStatement::relation); }
    };

    template <>
    struct FieldsOf<ir::QueryStatement> {
        static auto get() { return std::make_tuple(&ir::QueryStatement::relation); }
    };

    template <>
    struct FieldsOf<ir::Projector> {
        static auto get() { return std::make_tuple(&ir::Projector::expr); }
    };

    template <>
    struct FieldsOf<ir::EmptyRelation> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::ValuesRelation> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::ProjectionRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::ProjectionRelation::source, &ir::ProjectionRelation::projectors);
        }
    };

    template <>
    struct FieldsOf<ir::AggregateRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::AggregateRelation::source, &ir::AggregateRelation::aggregates);
        }
    };

    template <>
    struct FieldsOf<ir::GroupRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::GroupRelation::source,
                &ir::GroupRelation::aggregates,
                &ir::GroupRelation::group_list);
        }
    };

    template <>
    struct FieldsOf<ir::LimitRelation> {
        static auto get() { return std::make_tuple(&ir::LimitRelation::source); }
    };

    template <>
    struct FieldsOf<ir::FilterRelation> {
        static auto get() {
            return std::make_tuple(&ir::FilterRelation::source, &ir::FilterRelation::condition);
        }
    };

    template <>
    struct FieldsOf<ir::SortRelation> {
        static auto get() {
            return std::make_tuple(&ir::SortRelation::source, &ir::SortRelation::order_list);
        }
    };

    template <>
    struct FieldsOf<ir::TopKRelation> {
        static auto get() {
            return std::make_tuple(&ir::TopKRelation::source, &ir::TopKRelation::order_list);
        }
    };

    template <>
    struct FieldsOf<ir::SemiJoinRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::SemiJoinRelation::source,
                &ir::SemiJoinRelation::match,
                &ir::SemiJoinRelation::expr);
        }
    };

    template <>
    struct FieldsOf<ir::MarkJoinRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::MarkJoinRelation::source,
                &ir::MarkJoinRelation::match,
                &ir::MarkJoinRelation::expr);
        }
    };

    template <>
    struct FieldsOf<ir::UnionAllRelation> {
        static auto get() {
            return std::make_tuple(&ir::UnionAllRelation::left, &ir::UnionAllRelation::right);
        }
    };

    template <>
    struct FieldsOf<ir::UnionAllSortedByRelation> {
        static auto get() {
            return std::make_tuple(
                &ir::UnionAllSortedByRelation::left,
                &ir::UnionAllSortedByRelation::right,
                &ir::UnionAllSortedByRelation::order_list);
        }
    };

    template <>
    struct FieldsOf<ir::FileRelation> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::FileIntervalRelation> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::NamedRelationReferenceRelation> {
        static auto get() { return std::make_tuple(); }
    };

    template <>
    struct FieldsOf<ir::MaterializeRelation> {
        static auto get() { return std::make_tuple(&ir::MaterializeRelation::source); }
    };
};

template <typename Self>
class ConsumePass {
 public:
    ir::Relation pass(ir::Relation rel) {
        return util::match(rel.node, [&](auto& r) -> ir::Relation { return pass(r, rel); });
    }

    ir::Statement pass(ir::Statement st) {
        return util::match(st, [&](auto& s) -> ir::Statement { return pass(s, st); });
    }

    ir::Scalar pass(ir::Scalar sc) {
        return util::match(sc.node, [&](auto& s) -> ir::Scalar { return pass(s, sc); });
    }

    ir::Aggregate pass(ir::Aggregate ag) {
        return util::match(ag.node, [&](auto& s) -> ir::Aggregate { return pass(s, ag); });
    }

 private:
    template <typename E>
    E pass(auto& node, E& self) {
        using Fields = IRTree::template FieldsOf<std::decay_t<decltype(node)>>;
        auto modify = [&](auto field) { node.*field = pass(std::move(node.*field)); };
        std::apply([&](auto&&... fields) { (modify(fields), ...); }, Fields::get());
        return static_cast<Self*>(this)->construct(node, self);
    }

    template <typename E>
    Box<E> pass(Box<E> x) {
        return box(pass(std::move(*x)));
    }

    std::vector<ir::Scalar> pass(std::vector<ir::Scalar> ag) {
        std::vector<ir::Scalar> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(std::move(a)));
        }
        return r;
    }

    std::vector<ir::Aggregate> pass(std::vector<ir::Aggregate> ag) {
        std::vector<ir::Aggregate> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(std::move(a)));
        }
        return r;
    }

    std::vector<ir::Projector> pass(std::vector<ir::Projector> ag) {
        std::vector<ir::Projector> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back({
                .alias_field_id = a.alias_field_id,
                .expr = box(pass(std::move(*a.expr))),
            });
        }
        return r;
    }
};

template <typename Self, typename R>
class ScalarViewPass {
 public:
    R pass(const ir::Scalar& sc) {
        return util::match(sc.node, [&](auto& s) -> R { return pass(s, sc); });
    }

 private:
    template <typename E>
    R pass(auto& node, E& self) {
        using Fields = IRTree::template FieldsOf<std::decay_t<decltype(node)>>;
        auto do_view = [&](auto field) { return pass(node.*field); };
        return std::apply(
            [&](auto&&... fields) {
                return static_cast<Self*>(this)->view(node, self, do_view(fields)...);
            },
            Fields::get());
    }

    template <typename E>
    R pass(const Box<E>& x) {
        return pass(*x);
    }

    std::vector<R> pass(const std::vector<ir::Scalar>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(a));
        }
        return r;
    }

    std::vector<R> pass(const std::vector<ir::Aggregate>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(a));
        }
        return r;
    }

    std::vector<R> pass(const std::vector<ir::Projector>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(*a.expr));
        }
        return r;
    }
};

}  // namespace lsql::opt
