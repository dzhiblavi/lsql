#pragma once

#include "ir/Expressions.h"  // IWYU pragma: keep
#include "ir/Relations.h"    // IWYU pragma: keep
#include "ir/Statement.h"    // IWYU pragma: keep

#include "util/StrBuilder.h"

#include <magic_enum/magic_enum.hpp>

namespace lsql::ir {

class Stringifier {
    using StrBuilder = util::StrBuilder;

 public:
    Stringifier() = default;

    std::string print(const Program& program) {
        binding_ = program.field_binding;
        StrBuilder b("Program IR");

        for (auto&& s : program.statements) {
            b.child(print(s));
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
        return StrBuilder("NamedRelationStatement (name={})", s.name).child(print(*s.relation));
    }

    StrBuilder print(const Relation& r) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, r);
    }
    StrBuilder print(const ValuesRelation& r) {
        return StrBuilder()
            .line(
                "ValuesRelation count={}, type={}",
                r.values.size(),
                magic_enum::enum_name(r.values.empty() ? ValueType::Null : r.values.front().type()))
            .child(StrBuilder("values").child(toString(r.values)));
    }

    StrBuilder print(const ProjectionRelation& r) {
        return StrBuilder("ProjectionRelation")
            .child(StrBuilder("source").child(print(*r.source)))
            .child(StrBuilder("projectors").child(print(r.projectors)));
    }

    StrBuilder print(const AggregateRelation& r) {
        return StrBuilder("AggregateRelation")
            .child(StrBuilder("source").child(print(*r.source)))
            .child(StrBuilder("projectors").child(print(r.projectors)));
    }

    StrBuilder print(const GroupRelation& r) {
        return StrBuilder("GroupRelation")
            .child(StrBuilder("source").child(print(*r.source)))
            .child(StrBuilder("projectors").child(print(r.projectors)))
            .child(StrBuilder("group keys").child(print(r.group_list)));
    }

    StrBuilder print(const LimitRelation& r) {
        return StrBuilder("LimitRelation count={}", r.limit).child(print(*r.source));
    }

    StrBuilder print(const FilterRelation& r) {
        return StrBuilder("FilterRelation")
            .child(StrBuilder("source").child(print(*r.source)))
            .child(StrBuilder("condition").child(print(*r.condition)));
    }

    StrBuilder print(const SortRelation& r) {
        return StrBuilder("SortRelation")
            .child(StrBuilder("source").child(print(*r.source)))
            .child(StrBuilder("order list").child(print(r.order_list)));
    }

    StrBuilder print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    StrBuilder print(const StarProjector&) { return StrBuilder("*-projector"); }

    StrBuilder print(const ExprProjector& p) {
        return StrBuilder(
                   "ExprProjector alias_field_id={} alias_name={}",
                   p.alias_field_id,
                   binding_->name(p.alias_field_id))
            .child(print(*p.expr));
    }

    StrBuilder print(const UnionAllRelation& r) {
        return StrBuilder("UnionAll")
            .child(StrBuilder("- left").child(print(*r.left)))
            .child(StrBuilder("- right").child(print(*r.right)));
    }

    StrBuilder print(const UnionAllSortedByRelation& r) {
        return StrBuilder("UnionAll")
            .child(StrBuilder("- left").child(print(*r.left)))
            .child(StrBuilder("- right").child(print(*r.right)))
            .child(StrBuilder("order list").child(print(r.order_list)));
    }

    StrBuilder print(const std::vector<Projector>& ps) {
        auto b = StrBuilder();
        for (auto&& p : ps) {
            b.block(print(p));
        }
        return b;
    }

    StrBuilder print(const std::vector<Expr>& es) {
        auto b = StrBuilder();
        for (auto&& e : es) {
            b.block(print(e));
        }
        return b;
    }

    StrBuilder print(const FileRelation& r) { return StrBuilder("FileRelation path={}", r.path); }

    StrBuilder print(const FileIntervalRelation& r) {
        return StrBuilder("FileIntervalRelation path={} range=[{},{}]", r.path, r.ts_from, r.ts_to);
    }

    StrBuilder print(const NamedRelationReferenceRelation& r) {
        return StrBuilder("ReferenceRelation name={}", r.name);
    }

    StrBuilder print(const MaterializeRelation& r) {
        return StrBuilder("MaterializeRelation").child(print(*r.relation));
    }

    StrBuilder print(const Expr& e) {
        return std::visit(
            [this, &e](auto&& arg) {
                return this->print(arg)
                    .child(StrBuilder("value_type: {}", magic_enum::enum_name(valueTypeOf(e))))
                    .child(StrBuilder(
                        "expr_kind_level: {}", magic_enum::enum_name(exprKindLevelOf(e))));
            },
            e);
    }

    StrBuilder print(const FieldExpr& e) {
        return StrBuilder("FieldExpr id={} name={}", e.field_id, binding_->name(e.field_id));
    }

    StrBuilder print(const ValueExpr& e) {
        return StrBuilder(
            "ValueExpr {} (type {})", to_string(e.value), magic_enum::enum_name(e.value.type()));
    }

    StrBuilder print(const InExpr& e) {
        return StrBuilder("InExpr")
            .child(StrBuilder("expression").child(print(*e.expr)))
            .child(StrBuilder("match source").child(print(*e.source)));
    }

    StrBuilder print(const CoalesceExpr& e) {
        return StrBuilder("CoalesceExpr type={}", magic_enum::enum_name(e.valueType()))
            .child(StrBuilder("expressions").child(print(e.args)));
    }

    StrBuilder print(const CastExpr& e) {
        return StrBuilder("CastExpr to type {}", magic_enum::enum_name(e.cast_to))
            .child(print(*e.expr));
    }

    StrBuilder print(const PercentileExpr& e) {
        return StrBuilder("PercentileExpr count={}", e.percentiles.size())
            .child(StrBuilder("percentiles").child(toString(e.percentiles)))
            .child(StrBuilder("expression").child(print(*e.expr)));
    }

    StrBuilder print(const LikeExpr& e) {
        return StrBuilder("LikeExpr '{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const RSubstrExpr& e) {
        return StrBuilder("RSubstrExpr regex='{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const BinaryExpr& e) {
        return StrBuilder("BinaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(StrBuilder("left").child(print(*e.left)))
            .child(StrBuilder("right").child(print(*e.right)));
    }

    StrBuilder print(const UnaryExpr& e) {
        return StrBuilder("UnaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }

    StrBuilder print(const UnaryAggregateExpr& e) {
        return StrBuilder("UnaryAggregateExpr type={}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }

    template <typename T>
    std::string toString(const std::vector<T>& values) {
        using std::to_string;

        std::stringstream ss;
        ss << '[';
        for (auto&& v : values) {
            ss << to_string(v) << ',';
        }
        if (!values.empty()) {
            ss.seekp(-1, std::ios_base::end);
        }
        ss << ']';
        return ss.str();
    }

    ConstFieldBindingPtr binding_;
};

}  // namespace lsql::ir
