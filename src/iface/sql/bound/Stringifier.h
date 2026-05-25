#pragma once

#include "iface/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bound/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bound/Statement.h"    // IWYU pragma: keep

#include "util/StrBuilder.h"
#include "util/string.h"

#include <magic_enum/magic_enum.hpp>

#include <format>

namespace lsql::iface::sql::bound {

class Stringifier {
    using StrBuilder = util::StrBuilder;

 public:
    Stringifier() = default;

    std::string print(const Program& program) {
        auto b = StrBuilder("Bound AST");
        binding_ = program.binding;
        for (auto&& s : program.statements) {
            b.item(print(s));
        }
        return b.render();
    }

 private:
    StrBuilder print(const Statement& st) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, st);
    }

    StrBuilder print(const QueryStatement& s) {
        return StrBuilder("QueryStatement").child(print(*s.relation));
    }

    StrBuilder print(const NamedRelationStatement& s) {
        return StrBuilder("NamedRelationStatement name={}", s.name).child(print(*s.relation));
    }

    StrBuilder print(const Relation& r) {
        return std::visit(
            [&](auto&& arg) {
                return this->print(arg).child(
                    StrBuilder("fields out")
                        .item(StrBuilder(
                            "relation: {}", to_string(r.fields_out->fieldSet(), *binding_)))
                        .item(StrBuilder(
                            "subtree: {}", to_string(r.fields_out->subtreeFieldSet(), *binding_))));
            },
            r.node);
    }

    StrBuilder print(const AdhocRelation& r) {
        return StrBuilder("AdhocRelation count={}", r.values.size())
            .child(StrBuilder("values").child(util::toString(r.values)))
            .child(StrBuilder("output_field: {}", to_string(r.output_field_id, *binding_)));
    }

    StrBuilder print(const SelectRelation& r) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : r.projectors) {
            p.item(print(proj));
        }

        auto b = StrBuilder("SelectRelation aggregate={}", r.aggregate)
                     .child(StrBuilder("source").child(print(*r.source)))
                     .child(p);

        if (r.where) {
            b.child(print(*r.where));
        }
        if (r.limit) {
            b.child(print(*r.limit));
        }
        if (r.order_by) {
            b.child(print(*r.order_by));
        }
        if (r.group_by) {
            b.child(print(*r.group_by));
        }

        return b;
    }

    StrBuilder print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    StrBuilder print(const StarProjector&) { return StrBuilder("*-projector"); }

    StrBuilder print(const IdentifierProjector& p) {
        return StrBuilder("IdentifierProjector {}", to_string(p.field_id, *binding_));
    }

    StrBuilder print(const ExprProjector& p) {
        return StrBuilder("ExprProjector {}", to_string(p.alias_field_id, *binding_))
            .child(print(*p.expr));
    }

    StrBuilder print(const Where& w) { return StrBuilder("Where").child(print(*w.condition)); }

    StrBuilder print(const Limit& l) { return StrBuilder("Limit={}", l.limit); }

    StrBuilder print(const OrderBy& o) {
        auto b = StrBuilder("OrderBy desc={}", o.desc);
        for (auto&& e : o.order_list) {
            b.item(print(e));
        }
        return b;
    }

    StrBuilder print(const GroupBy& g) {
        auto b = StrBuilder("GroupBy");
        for (auto&& p : g.group_list) {
            b.item(print(p));
        }
        return b;
    }

    StrBuilder print(const UnionAllRelation& r) {
        return StrBuilder("UnionAllRelation")
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)));
    }

    StrBuilder print(const UnionAllSortedByRelation& r) {
        auto order = StrBuilder("order list");
        for (auto&& item : r.order_by.order_list) {
            order.item(print(item));
        }

        return StrBuilder("UnionAllSortedByRelation desc={}", r.order_by.desc)
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)))
            .child(order);
    }

    StrBuilder print(const FileRelation& r) { return StrBuilder("FileRelation path={}", r.path); }

    StrBuilder print(const FileIntervalRelation& r) {
        return StrBuilder("FileIntervalRelation path={} range=[{},{}]", r.path, r.ts_from, r.ts_to);
    }

    StrBuilder print(const NamedRelationReferenceRelation& r) {
        return StrBuilder("NamedRelationReferenceRelation name={}", r.name);
    }

    StrBuilder print(const MaterializeRelation& r) {
        return StrBuilder("MaterializeRelation").child(print(*r.relation));
    }

    StrBuilder print(const Expr& e) {
        return std::visit(
            [&](auto&& arg) {
                return this->print(arg)
                    .child(StrBuilder("value_type: {}", magic_enum::enum_name(e.value_type)))
                    .child(StrBuilder("level: {}", magic_enum::enum_name(e.level)))
                    .child(StrBuilder("req_fields: {}", to_string(e.required_fields, *binding_)));
            },
            e.node);
    }

    StrBuilder print(const IdentifierExpr& e) {
        return StrBuilder("IdentifierExpr {}", to_string(e.field_id, *binding_));
    }

    StrBuilder print(const ValueExpr& e) { return StrBuilder("ValueExpr {}", to_string(e.value)); }

    StrBuilder print(const CastExpr& e) {
        return StrBuilder("CastExpr to type {}", magic_enum::enum_name(e.cast_to))
            .child(print(*e.expr));
    }

    StrBuilder print(const InExpr& e) {
        return StrBuilder("InExpr")
            .child(StrBuilder("expression").child(print(*e.expr)))
            .child(StrBuilder("match set").child(print(*e.match)))
            .child(StrBuilder("match field id: {}", to_string(e.match_field_id, *binding_)));
    }

    StrBuilder print(const LikeExpr& e) {
        return StrBuilder("LikeExpr regex='{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const CoalesceExpr& e) {
        auto b = StrBuilder("args");
        for (auto&& a : e.args) {
            b.item(print(a));
        }
        return StrBuilder("CoalesceExpr").child(b);
    }

    StrBuilder print(const PercentileExpr& e) {
        return StrBuilder("PercentileExpr count={}", e.percentiles.size())
            .child(StrBuilder("percentiles").child(util::toString(e.percentiles)))
            .child(StrBuilder("expression").child(print(*e.expr)));
    }

    StrBuilder print(const RSubstrExpr& e) {
        return StrBuilder("RSubstrExpr regex='{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const BinaryExpr& e) {
        return StrBuilder("BinaryExpr type: {}", magic_enum::enum_name(e.type))
            .item(StrBuilder("left").child(print(*e.left)))
            .item(StrBuilder("right").child(print(*e.right)));
    }

    StrBuilder print(const UnaryExpr& e) {
        return StrBuilder("UnaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }

    StrBuilder print(const UnaryAggregateExpr& e) {
        return StrBuilder("UnaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }

 private:
    ConstFieldBindingPtr binding_;
};

}  // namespace lsql::iface::sql::bound
