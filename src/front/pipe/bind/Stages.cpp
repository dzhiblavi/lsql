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
            requireAt(maybe_type.has_value(), pp.span, "unknown field '{}'", name);
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
    auto order_list_span = spanOf(r.order_list);
    auto order_list = bindExprs(std::move(r.order_list), ctx);
    requireAt(!order_list.empty(), self.span, "order list cannot be empty");

    for (auto&& e : order_list) {
        requireAt(
            e.level != ExprKindLevel::Group,
            order_list_span,
            "sort by cannot use aggregate expression here");
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

bound::Stage bindStage(ast::SelectStage r, auto&& /*self*/, Context& ctx) {
    auto projectors_span = spanOf(r.projectors);
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    requireAt(!projectors.empty(), projectors_span, "select requires at least one projector");

    bool has_aggregates = false;
    bool has_scalars = false;
    for (auto&& p : projectors) {
        util::match(
            p,
            [&](const bound::StarProjector&) { /* nothing to check */ },
            [&](const bound::IdentifierProjector&) { has_scalars = true; },
            [&](const bound::ExprProjector& p) {
                has_scalars |= p.expr->level == ExprKindLevel::Row;
                has_aggregates |= p.expr->level == ExprKindLevel::Group;
            });
    }
    requireAt(
        !(has_scalars && has_aggregates),
        projectors_span,
        "select: cannot mix aggregates and scalars");

    auto output_fields = outputFieldsOf(projectors);
    return {
        .node = bound::SelectStage{.projectors = std::move(projectors)},
        .fields_out = FieldSetNode::make(output_fields, ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::GroupStage r, auto&& /*self*/, Context& ctx) {
    auto group_list_span = spanOf(r.group_list);
    auto group_list = bindProjectors<bound::Projector>(std::move(r.group_list), bindProjector, ctx);
    requireAt(!group_list.empty(), group_list_span, "group list cannot be empty");
    auto group_list_fields = outputFieldsOf(group_list);

    auto projectors_span = spanOf(r.projectors);
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    requireAt(!projectors.empty(), group_list_span, "group projectors are required");
    auto projectors_fields = outputFieldsOf(projectors);

    auto group_list_map = buildMap(group_list);
    for (auto&& p : projectors) {
        util::match(
            p,
            [&](const bound::StarProjector&) { /* ok, just all group keys*/ },
            [&](const bound::IdentifierProjector& p) {
                requireAt(
                    group_list_map.contains(p.field_id),
                    projectors_span,
                    "group by: unknown field id {}",
                    p.field_id);
            },
            [&](const bound::ExprProjector& p) {
                if (p.expr->level != ExprKindLevel::Row) {
                    // Ok (Const/Group projectors)
                    return;
                }
                for (auto id : p.expr->required_fields.fieldIds()) {
                    requireAt(
                        group_list_map.contains(id),
                        projectors_span,
                        "group by: unknown field id {} required by expression",
                        id);
                }
            });
    }

    return {
        .node =
            bound::GroupStage{
                .projectors = std::move(projectors),
                .group_list = std::move(group_list),
            },
        .fields_out = FieldSetNode::make(projectors_fields, ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::Stage st, Context& ctx) {
    return util::match(
        std::move(st.node), [&](auto node) { return bindStage(std::move(node), st, ctx); });
}

}  // namespace lsql::front::pipe::bind
