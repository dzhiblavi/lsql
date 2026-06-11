#include "front/pipe/bind/Stages.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"

#include "front/pipe/bind/Expressions.h"
#include "front/pipe/bind/Pipeline.h"
#include "front/pipe/bind/helpers.h"

#include "front/pipe/bound/Expressions.h"
#include "front/pipe/bound/Sources.h"  // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"

#include "front/common/bind/helpers.h"
#include "front/common/source/require_at.h"

namespace lsql::front::pipe::bind {

namespace {

using common::bind::FieldSetChain;
using common::bound::ExprKindLevel;
using common::bound::FieldSetNode;
using common::bound::FieldSetNodePtr;

void bindProjector(ast::Projector pp, std::vector<bound::Projector>& out, Context& ctx) {
    util::match(
        std::move(pp.node),
        [&](ast::StarProjector) { out.emplace_back(bound::StarProjector{}); },
        [&](ast::IdentifierProjector p) {
            auto name = p.identifier.substr(1);
            auto maybe_type = ctx.currFieldSet().typeOfSourceField(name, ctx.binding());
            requireAt(maybe_type.has_value(), pp.span, "unknown identifier '{}'", name);
            auto id = ctx.binding()->getOrAdd(name, *maybe_type);
            out.emplace_back(bound::IdentifierProjector{.field_id = id});
        },
        [&](ast::ExprProjector p) {
            auto expr = bindExpr(std::move(*p.expr), ctx);

            out.emplace_back(
                bound::ExprProjector{
                    .alias_field_id = ctx.binding()->getOrAdd(p.alias, expr.value_type),
                    .expr = box(std::move(expr)),
                });
        });
}

}  // namespace

bound::Stage bindStage(ast::FilterStage r, auto&& /*self*/, Context& ctx) {
    auto cond_span = r.condition->span;
    auto cond = bindExpr(std::move(*r.condition), ctx);
    requireAt(cond.value_type == ValueType::Boolean, cond_span, "WHERE condition must be boolean");
    requireAt(cond.level != ExprKindLevel::Group, cond_span, "WHERE condition cannot be aggregate");

    return {
        .node = bound::FilterStage{.condition = box(std::move(cond))},
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::WhereInStage r, auto&& /*self*/, Context& ctx) {
    auto expr_span = r.expr->span;
    auto expr = bindExpr(std::move(*r.expr), ctx);
    requireAt(
        expr.level != ExprKindLevel::Group, expr_span, "in key expression cannot be aggregate");

    auto match_span = r.match->span;
    auto match = bindPipeline(std::move(*r.match), ctx);
    auto count = match.fields_out->fieldSet().fieldIds().size();
    requireAt(count == 1, match_span, "in match should contain one column, got {}", count);
    auto match_field_id = *match.fields_out->fieldSet().fieldIds().begin();
    requireAt(
        expr.value_type == ctx.binding()->type(match_field_id), expr_span, "in key type mismatch");

    return {
        .node =
            bound::WhereInStage{
                .expr = box(std::move(expr)),
                .match = box(std::move(match)),
                .match_field_id = match_field_id,
            },
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::TakeStage r, auto&& self, Context& ctx) {
    requireAt(r.count >= 0, self.span, "take count cannot be negative");

    return {
        .node = bound::TakeStage{.count = r.count},
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::SortStage r, auto&& self, Context& ctx) {
    auto order_list_span = spansOf(r.order_list);
    auto order_list = bindExprs(std::move(r.order_list), ctx);
    requireAt(!order_list.empty(), self.span, "order list cannot be empty");

    for (auto&& [e, span] : std::views::zip(order_list, order_list_span)) {
        requireAt(
            e.level != ExprKindLevel::Group, span, "sort by cannot use aggregate expression here");
    }

    return {
        .node =
            bound::SortStage{
                .order_list = std::move(order_list),
                .desc = r.desc,
            },
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::SelectStage r, auto&& self, Context& ctx) {
    auto projectors_spans = spansOf(r.projectors);
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    requireAt(!projectors.empty(), self.span, "select requires at least one projector");

    bool has_aggregates = false;
    bool has_scalars = false;
    bool has_star = false;
    for (auto&& [p, span] : std::views::zip(projectors, projectors_spans)) {
        util::match(
            p,
            [&](const bound::StarProjector&) {
                if (has_aggregates) {
                    throwAt(span, "cannot mix aggregates and scalars");
                }
                has_star = true;
                has_scalars = true;
            },
            [&](const bound::IdentifierProjector&) {
                if (has_aggregates) {
                    throwAt(span, "cannot mix aggregates and scalars");
                }
                has_scalars = true;
            },
            [&](const bound::ExprProjector& p) {
                if (p.expr->level == ExprKindLevel::Row) {
                    if (has_aggregates) {
                        throwAt(span, "cannot mix aggregates and scalars");
                    }
                    has_scalars = true;
                }
                if (p.expr->level == ExprKindLevel::Group) {
                    if (has_scalars) {
                        throwAt(span, "cannot mix aggregates and scalars");
                    }
                    has_aggregates = true;
                }
            });
    }

    auto output_fields = outputFieldsOf(projectors);
    FieldSetNodePtr fields_out;

    if (has_aggregates) {
        fields_out = FieldSetNode::make(output_fields);
    } else {
        if (has_star) {
            fields_out = FieldSetNode::make(output_fields, ctx.currFieldSet().top());
        } else {
            fields_out = FieldSetNode::make(output_fields);
        }
    }

    return {
        .node = bound::SelectStage{.projectors = std::move(projectors)},
        .fields_out = fields_out,
    };
}

bound::Stage bindStage(ast::GroupStage r, auto&& /*self*/, Context& ctx) {
    auto group_list_span = spanOf(r.group_list);
    auto group_list = bindProjectors<bound::Projector>(std::move(r.group_list), bindProjector, ctx);
    requireAt(!group_list.empty(), group_list_span, "group list cannot be empty");
    auto group_list_fields = outputFieldsOf(group_list);

    auto projectors_spans = spansOf(r.projectors);
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    requireAt(!projectors.empty(), group_list_span, "group projectors are required");
    auto projectors_fields = outputFieldsOf(projectors);

    auto group_list_map = buildMap(group_list);
    for (auto&& [p, span] : std::views::zip(projectors, projectors_spans)) {
        util::match(
            p,
            [&](const bound::StarProjector&) { /* ok, just all group keys*/ },
            [&](const bound::IdentifierProjector& p) {
                requireAt(
                    group_list_map.contains(p.field_id),
                    span,
                    "group by: unknown identifier '{}'",
                    to_string(p.field_id, *ctx.binding()));
            },
            [&](const bound::ExprProjector& p) {
                if (p.expr->level != ExprKindLevel::Row) {
                    // Ok (Const/Group projectors)
                    return;
                }
                for (auto id : p.expr->required_fields.fieldIds()) {
                    requireAt(
                        group_list_map.contains(id),
                        span,
                        "group by: unknown identifier '{}'",
                        to_string(id, *ctx.binding()));
                }
            });
    }

    return {
        .node =
            bound::GroupStage{
                .projectors = std::move(projectors),
                .group_list = std::move(group_list),
            },
        .fields_out = FieldSetNode::make(projectors_fields),
    };
}

bound::Stage bindStage(ast::Stage st, Context& ctx) {
    return util::match(
        std::move(st.node), [&](auto node) { return bindStage(std::move(node), st, ctx); });
}

}  // namespace lsql::front::pipe::bind
