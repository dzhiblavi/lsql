#include "iface/sql/bind/Relations.h"

#include "iface/sql/bind/Expressions.h"
#include "iface/sql/bind/helpers.h"

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"

#include "iface/sql/bound/Expressions.h"
#include "iface/sql/bound/Relations.h"

#include "core/time_formats.h"

namespace lsql::iface::sql::bind {

namespace {

void bindProjector(ast::Projector p, std::vector<bound::Projector>& out, Context& ctx) {
    util::match(
        std::move(p),
        [&](ast::StarProjector) { out.emplace_back(bound::StarProjector{}); },
        [&](ast::IdentifierProjector p) {
            auto type = ctx.currFieldSet().typeOfSourceField(p.identifier, ctx.binding());
            auto id = ctx.binding()->getOrAdd(p.identifier, type);
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

std::vector<bound::Projector> bindProjectors(std::vector<ast::Projector> projectors, Context& ctx) {
    std::vector<bound::Projector> result;
    result.reserve(projectors.size());
    for (auto&& p : projectors) {
        bindProjector(std::move(p), result, ctx);
    }
    return result;
}

}  // namespace

bound::Relation bindRelation(ast::AdhocRelation r, Context& ctx) {
    std::vector<Value> values;
    values.reserve(r.literals.size());
    for (auto&& literal : r.literals) {
        values.push_back(parseLiteral(literal));
        require(
            values.back().type() == values.front().type(),
            "Ad hoc relation should contain entries of the same type");
    }

    auto type = values.empty() ? ValueType::Null : values.front().type();
    auto id = ctx.binding()->getOrAdd("anon", type);

    return {
        .node =
            bound::AdhocRelation{
                .values = std::move(values),
                .output_field_id = id,
            },
        .fields_out = bound::FieldSetNode::make(FieldSet::withField(id)),
    };
}

bound::Relation bindRelation(ast::SelectRelation r, Context& ctx) {
    auto source = bindRelation(std::move(*r.source), ctx);
    auto source_field_set_node = source.fields_out;

    auto generated_visible_fields = bound::FieldSetNode::emptySet();
    auto source_visible_fields = FieldSetChain(source_field_set_node.get(), nullptr);
    auto visible_fields = FieldSetChain(generated_visible_fields.get(), &source_visible_fields);
    auto _ = ctx.scopedFieldSet(&visible_fields);

    std::optional<bound::Where> where;
    if (r.where) {
        auto cond = bindExpr(std::move(*r.where->condition), ctx);
        require(cond.value_type == ValueType::Boolean, "WHERE condition must be boolean");
        require(cond.level != ExprKindLevel::Group, "WHERE condition cannot be aggregate");
        where = bound::Where{.condition = box<bound::Expr>(std::move(cond))};
    }

    std::optional<bound::Limit> limit;
    if (r.limit) {
        require(r.limit->limit > 0, "limit cannot be negative");
        limit = bound::Limit{.limit = r.limit->limit};
    }

    auto projectors = bindProjectors(std::move(r.projectors), ctx);
    require(!projectors.empty(), "SELECT requires at least one projector");
    auto output_fields = outputFieldsOf(projectors);
    bound::FieldSetNodePtr fields_out;

    bool has_group_by = r.group_by.has_value();
    bool has_group_projector = false;
    for (auto&& p : projectors) {
        util::match(
            p,
            [&](const bound::ExprProjector& p) {
                has_group_projector |= p.expr->level == ExprKindLevel::Group;
            },
            [&](const bound::IdentifierProjector&) {},
            [&](const bound::StarProjector&) {});
    }

    std::optional<bound::GroupBy> group_by;
    if (has_group_by) {
        // Group by
        auto group_key = bindProjectors(std::move(r.group_by->group_list), ctx);

        for (auto&& p : group_key) {
            util::matchPartial(p, [](const bound::StarProjector&) {
                throwError("Star projectors are not allowed in GROUP BY");
            });
        }

        auto group_key_map = buildMap(group_key);

        for (auto&& p : projectors) {
            util::match(
                p,
                [](const bound::StarProjector&) { /* ok, all group keys */ },
                [&](const bound::IdentifierProjector& p) {
                    require(
                        group_key_map.contains(p.field_id),
                        "GROUP BY: unknown field id {}",
                        p.field_id);
                },
                [&](const bound::ExprProjector& p) {
                    if (p.expr->level != ExprKindLevel::Row) {
                        // Ok (Const/Group projectors)
                        return;
                    }
                    for (auto id : p.expr->required_fields.fieldIds()) {
                        require(
                            group_key_map.contains(id),
                            "GROUP BY: unknown field id {} required by expression",
                            id);
                    }
                });
        }

        group_by = bound::GroupBy{.group_list = std::move(group_key)};

        // Add group output keys
        generated_visible_fields->merge(outputFieldsOf(group_key));
        fields_out = bound::FieldSetNode::make(output_fields);
    } else if (has_group_projector) {
        // Aggregate
        for (auto&& p : projectors) {
            util::match(
                p,
                [](const bound::StarProjector&) {
                    throwError("Star projectors are not allowed in aggregates");
                },
                [](const bound::IdentifierProjector&) {
                    throwError("Identifier projectors are not allowed in aggregates");
                },
                [](const bound::ExprProjector& p) {
                    require(
                        p.expr->level != ExprKindLevel::Row,
                        "Row projectors are not allowed in aggregates");
                });
        }
        fields_out = bound::FieldSetNode::make(output_fields);
    } else {
        // Simple select, nothing left to check
        fields_out = bound::FieldSetNode::make(output_fields, source_field_set_node);
    }

    std::optional<bound::OrderBy> order_by;
    if (r.order_by) {
        generated_visible_fields->merge(output_fields);

        auto order_list = bindExprs(std::move(r.order_by->order_list), ctx);
        order_by = bound::OrderBy{
            .order_list = std::move(order_list),
            .desc = r.order_by->desc,
        };
    }

    return {
        .node =
            bound::SelectRelation{
                .projectors = std::move(projectors),
                .source = box(std::move(source)),
                .limit = std::move(limit),
                .where = std::move(where),
                .order_by = std::move(order_by),
                .group_by = std::move(group_by),
                .aggregate = has_group_projector,
            },
        .fields_out = std::move(fields_out),
    };
}

bound::Relation bindRelation(ast::UnionAllRelation r, Context& ctx) {
    auto left = bindRelation(std::move(*r.left), ctx);
    auto right = bindRelation(std::move(*r.right), ctx);
    auto fields = bound::FieldSetNode::proxy(left.fields_out, right.fields_out);

    return {
        .node =
            bound::UnionAllRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .fields_out = std::move(fields),
    };
}

bound::Relation bindRelation(ast::UnionAllSortedByRelation r, Context& ctx) {
    auto left = bindRelation(std::move(*r.left), ctx);
    auto right = bindRelation(std::move(*r.right), ctx);
    auto fields = bound::FieldSetNode::proxy(left.fields_out, right.fields_out);

    FieldSetChain fields_set(fields.get(), nullptr);
    auto _ = ctx.scopedFieldSet(&fields_set);

    auto order_list = bindExprs(std::move(r.order_by.order_list), ctx);
    require(!order_list.empty(), "order list cannot be empty");

    return {
        .node =
            bound::UnionAllSortedByRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
                .order_by =
                    bound::OrderBy{
                        .order_list = std::move(order_list),
                        .desc = r.order_by.desc,
                    },
            },
        .fields_out = std::move(fields),
    };
}

bound::Relation bindRelation(ast::FileRelation r, Context& /*ctx*/) {
    return {
        .node = bound::FileRelation{.path = std::move(r.path)},
        .fields_out = bound::FieldSetNode::unknownSet(),
    };
}

bound::Relation bindRelation(ast::FileIntervalRelation r, Context& /*ctx*/) {
    constexpr auto format = TimeFormat::ISO8601;
    auto ts_from = timestampFromString(r.ts_from, format);

    return {
        .node =
            bound::FileIntervalRelation{
                .path = std::move(r.path),
                .ts_from = ts_from,
                .ts_to = ts_from + r.interval_s,
            },
        .fields_out = bound::FieldSetNode::unknownSet(),
    };
}

bound::Relation bindRelation(ast::NamedRelationReferenceRelation r, Context& ctx) {
    auto child_node_ptr = ctx.findRelation(r.name)->fields_out;

    return {
        .node = bound::NamedRelationReferenceRelation{.name = std::move(r.name)},
        .fields_out = bound::FieldSetNode::proxy(child_node_ptr),
    };
}

bound::Relation bindRelation(ast::MaterializeRelation r, Context& ctx) {
    auto arg = bindRelation(std::move(*r.relation), ctx);
    auto fields = bound::FieldSetNode::proxy(arg.fields_out);

    return {
        .node = bound::MaterializeRelation{.relation = box(std::move(arg))},
        .fields_out = std::move(fields),
    };
}

bound::Relation bindRelation(ast::Relation rel, Context& ctx) {
    return util::match(std::move(rel), [&](auto r) { return bindRelation(std::move(r), ctx); });
}

}  // namespace lsql::iface::sql::bind
