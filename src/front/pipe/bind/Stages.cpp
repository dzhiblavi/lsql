#include "front/pipe/bind/Stages.h"

#include "front/common/bind/helpers.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"

#include "front/pipe/bind/Expressions.h"
#include "front/pipe/bind/Pipeline.h"
#include "front/pipe/bind/helpers.h"

#include "front/pipe/bound/Expressions.h"
#include "front/pipe/bound/Sources.h"  // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"

namespace lsql::front::pipe::bind {

namespace {

using common::bind::FieldSetChain;
using common::bound::ExprKindLevel;
using common::bound::FieldSetNode;
using common::bound::FieldSetNodePtr;

void bindProjector(ast::Projector p, std::vector<bound::Projector>& out, Context& ctx) {
    util::match(
        std::move(p),
        [&](ast::StarProjector) { out.emplace_back(bound::StarProjector{}); },
        [&](ast::IdentifierProjector p) {
            auto name = p.identifier.substr(1);
            auto type = ctx.currFieldSet().typeOfSourceField(name, ctx.binding());
            auto id = ctx.binding()->getOrAdd(name, type);
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

bound::Stage bindStage(ast::FilterStage r, Context& ctx) {
    auto cond = bindExpr(std::move(*r.condition), ctx);
    require(cond.value_type == ValueType::Boolean, "WHERE condition must be boolean");
    require(cond.level != ExprKindLevel::Group, "WHERE condition cannot be aggregate");

    return {
        .node = bound::FilterStage{.condition = box(std::move(cond))},
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::WhereInStage r, Context& ctx) {
    auto expr = bindExpr(std::move(*r.expr), ctx);
    require(expr.level != ExprKindLevel::Group, "in key expression cannot be aggregate");

    auto match = bindPipeline(std::move(*r.match), ctx);
    auto count = match.fields_out->fieldSet().fieldIds().size();
    verify(count == 1, "expected 1, got {}", count);
    require(count == 1, "in match should contain one column, got {}", count);
    auto match_field_id = *match.fields_out->fieldSet().fieldIds().begin();
    require(expr.value_type == ctx.binding()->type(match_field_id), "in key type mismatch");

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

bound::Stage bindStage(ast::TakeStage r, Context& ctx) {
    require(r.count >= 0, "take count cannot be negative");

    return {
        .node = bound::TakeStage{.count = r.count},
        .fields_out = FieldSetNode::proxy(ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::SortStage r, Context& ctx) {
    auto order_list = bindExprs(std::move(r.order_list), ctx);
    require(!order_list.empty(), "sort order list cannot be empty");

    for (auto&& e : order_list) {
        require(e.level != ExprKindLevel::Group, "sort by cannot use aggregate expression here");
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

bound::Stage bindStage(ast::SelectStage r, Context& ctx) {
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    require(!projectors.empty(), "select requires at least one projector");

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
    require(!(has_scalars && has_aggregates), "select: cannot mix aggregates and scalars");

    auto output_fields = outputFieldsOf(projectors);
    return {
        .node = bound::SelectStage{.projectors = std::move(projectors)},
        .fields_out = FieldSetNode::make(output_fields, ctx.currFieldSet().top()),
    };
}

bound::Stage bindStage(ast::GroupStage r, Context& ctx) {
    auto group_list = bindProjectors<bound::Projector>(std::move(r.group_list), bindProjector, ctx);
    require(!group_list.empty(), "group list cannot be empty");
    auto group_list_fields = outputFieldsOf(group_list);

    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    require(!projectors.empty(), "group projectors are required");
    auto projectors_fields = outputFieldsOf(projectors);

    auto group_list_map = buildMap(group_list);
    for (auto&& p : projectors) {
        util::match(
            p,
            [&](const bound::StarProjector&) { /* ok, just all group keys*/ },
            [&](const bound::IdentifierProjector& p) {
                require(
                    group_list_map.contains(p.field_id),
                    "group by: unknown field id {}",
                    p.field_id);
            },
            [&](const bound::ExprProjector& p) {
                if (p.expr->level != ExprKindLevel::Row) {
                    // Ok (Const/Group projectors)
                    return;
                }
                for (auto id : p.expr->required_fields.fieldIds()) {
                    require(
                        group_list_map.contains(id),
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

bound::Stage bindStage(ast::Stage rel, Context& ctx) {
    return util::match(std::move(rel), [&](auto r) { return bindStage(std::move(r), ctx); });
}

}  // namespace lsql::front::pipe::bind
