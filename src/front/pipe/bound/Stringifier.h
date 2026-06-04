#pragma once

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/Sources.h"
#include "front/pipe/bound/Stages.h"
#include "front/pipe/bound/Statements.h"

#include "util/StrBuilder.h"
#include "util/string.h"

#include <magic_enum/magic_enum.hpp>

#include <format>

namespace lsql::front::pipe::bound {

class Stringifier {
    using StrBuilder = util::StrBuilder;

 public:
    Stringifier() = default;

    std::string print(const Program& p) {
        binding_ = p.binding;
        auto b = StrBuilder("Program");
        for (auto&& st : p.statements) {
            b.item(print(st));
        }
        return b.render();
    }

 private:
    StrBuilder print(const Statement& s) {
        return std::visit([this](auto&& p) { return print(p); }, s);
    }

    StrBuilder print(const QueryStatement& s) {
        return StrBuilder("Query").child(print(*s.pipeline));
    }

    StrBuilder print(const NamedPipelineStatement& s) {
        return StrBuilder("NamedPipeline name={}", s.name).child(print(*s.pipeline));
    }

    StrBuilder print(const Pipeline& p) {
        auto b = print(*p.source);
        for (auto&& s : p.stages) {
            b.item(print(s));
        }
        return b;
    }

    StrBuilder print(const Source& s) {
        return std::visit(
            [&](auto&& arg) {
                return this->print(arg).child(
                    StrBuilder("fields out")
                        .item(
                            StrBuilder("this: {}", to_string(s.fields_out->fieldSet(), *binding_)))
                        .item(StrBuilder(
                            "subtree: {}", to_string(s.fields_out->subtreeFieldSet(), *binding_))));
            },
            s.node);
    }

    StrBuilder print(const AdhocSource& r) {
        return StrBuilder("Adhoc count={}", r.values.size())
            .child(StrBuilder("values").child(util::toString(r.values)))
            .child(StrBuilder("output_field_id").child(to_string(r.output_field_id, *binding_)));
    }

    StrBuilder print(const NamedPipelineReferenceSource& r) {
        return StrBuilder("NamedPipelineReference name: {}", r.name);
    }

    StrBuilder print(const FileSource& r) { return StrBuilder("FileSource path={}", r.path); }

    StrBuilder print(const FileIntervalSource& r) {
        return StrBuilder(
            "FileIntervalSource path={} ts_from={}, ts_to={}", r.path, r.ts_from, r.ts_to);
    }

    StrBuilder print(const UnionAllSource& r) {
        return StrBuilder("UnionAll")
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)));
    }

    StrBuilder print(const UnionAllSortedBySource& r) {
        auto order = StrBuilder("order list");
        for (auto&& item : r.order_list) {
            order.item(print(item));
        }

        return StrBuilder("UnionAllSortedBy desc={}", r.desc)
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)))
            .child(order);
    }

    StrBuilder print(const Stage& st) {
        return std::visit(
            [&](auto&& arg) {
                return this->print(arg).child(
                    StrBuilder("fields out")
                        .item(
                            StrBuilder("this: {}", to_string(st.fields_out->fieldSet(), *binding_)))
                        .item(StrBuilder(
                            "subtree: {}",
                            to_string(st.fields_out->subtreeFieldSet(), *binding_))));
            },
            st.node);
    }

    StrBuilder print(const FilterStage& f) {
        return StrBuilder("Filter").child(print(*f.condition));
    }

    StrBuilder print(const TakeStage& t) { return StrBuilder("Take={}", t.count); }

    StrBuilder print(const SortStage& s) {
        auto b = StrBuilder("OrderBy desc={}", s.desc);
        for (auto&& e : s.order_list) {
            b.item(print(e));
        }
        return b;
    }

    StrBuilder print(const GroupStage& s) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : s.projectors) {
            p.item(print(proj));
        }
        auto g = StrBuilder("group key");
        for (auto&& proj : s.group_list) {
            g.item(print(proj));
        }
        return StrBuilder("Group").child(p).child(g);
    }

    StrBuilder print(const WhereInStage& s) {
        return StrBuilder("WhereIn")
            .child(StrBuilder("expr").block(print(*s.expr)))
            .child(StrBuilder("match").block(print(*s.match)));
    }

    StrBuilder print(const SelectStage& r) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : r.projectors) {
            p.item(print(proj));
        }
        return StrBuilder("Select").child(p);
    }

    StrBuilder print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    StrBuilder print(const StarProjector& /*p*/) { return StrBuilder("StarProjector"); }

    StrBuilder print(const IdentifierProjector& p) {
        return StrBuilder("IdentifierProjector field={}", to_string(p.field_id, *binding_));
    }

    StrBuilder print(const ExprProjector& p) {
        return StrBuilder("ExprProjector field={}", to_string(p.alias_field_id, *binding_))
            .child(print(*p.expr));
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
        return StrBuilder("IdentifierExpr field={}", to_string(e.field_id, *binding_));
    }

    StrBuilder print(const ValueExpr& e) {
        return StrBuilder("ValueExpr value={}", to_string(e.value));
    }

    StrBuilder print(const CastExpr& e) {
        return StrBuilder("CastExpr to type {}", magic_enum::enum_name(e.cast_to))
            .child(print(*e.expr));
    }

    StrBuilder print(const InExpr& e) {
        return StrBuilder("InExpr")
            .child(StrBuilder("expr").block(print(*e.expr)))
            .child(StrBuilder("match").block(print(*e.match)))
            .child(StrBuilder("match_field_id={}", to_string(e.match_field_id, *binding_)));
    }

    StrBuilder print(const LikeExpr& e) {
        return StrBuilder("LikeExpr '{}'", e.regex).child(print(*e.expr));
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

    StrBuilder print(const CountAllExpr&) { return StrBuilder("CountAllExpr"); }

    StrBuilder print(const UnaryAggregateExpr& e) {
        return StrBuilder("UnaryAggregateExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }

    ConstFieldBindingPtr binding_;
};

}  // namespace lsql::front::pipe::bound
