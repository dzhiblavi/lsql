#include "front/sql/bind/Relations.h"

#include "front/sql/bind/Expressions.h"
#include "front/sql/bind/helpers.h"

#include "front/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "front/sql/ast/Relations.h"

#include "front/sql/bound/Expressions.h"
#include "front/sql/bound/Relations.h"

#include "front/common/bind/helpers.h"
#include "front/common/source/require_at.h"

#include "core/time_formats.h"

namespace lsql::front::sql::bind {

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
            auto maybe_type = ctx.currFieldSet().typeOfSourceField(p.identifier, ctx.binding());
            requireAt(maybe_type.has_value(), pp.span, "unknown identifier '{}'", p.identifier);
            auto id = ctx.binding()->getOrAdd(p.identifier, *maybe_type);
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

bound::Relation bindRelation(ast::AdhocRelation r, auto&& self, Context& ctx) {
    std::vector<Value> values;
    values.reserve(r.literals.size());
    for (auto&& literal : r.literals) {
        auto value = parseLiteral(literal);
        requireAt(value.has_value(), literal.span, "invalid literal '{}'", literal.value_str);

        values.push_back(*value);
        requireAt(
            values.back().type() == values.front().type(),
            self.span,
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
        .fields_out = FieldSetNode::make(FieldSet::withField(id)),
    };
}

bound::Relation bindRelation(ast::SelectRelation r, auto&& self, Context& ctx) {
    auto source = bindRelation(std::move(*r.source), ctx);
    auto source_field_set_node = source.fields_out;

    auto generated_visible_fields = common::bound::FieldSetNode::emptySet();
    auto source_visible_fields = common::bind::FieldSetChain(source_field_set_node, nullptr);
    auto visible_fields =
        common::bind::FieldSetChain(generated_visible_fields, &source_visible_fields);
    auto _ = ctx.scopedFieldSet(&visible_fields);

    std::optional<bound::Where> where;
    if (r.where) {
        auto cond = bindExpr(std::move(*r.where->condition), ctx);
        requireAt(
            cond.value_type == ValueType::Boolean,
            r.where->span,
            "WHERE condition must be boolean");
        requireAt(
            cond.level != common::bound::ExprKindLevel::Group,
            r.where->span,
            "WHERE condition cannot be aggregate");

        where = bound::Where{.condition = box(std::move(cond))};
    }

    std::optional<bound::Limit> limit;
    if (r.limit) {
        requireAt(r.limit->limit > 0, r.limit->span, "limit cannot be negative");
        limit = bound::Limit{.limit = r.limit->limit};
    }

    auto projectors_span = spanOf(r.projectors);
    auto projectors = bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
    requireAt(!projectors.empty(), projectors_span, "SELECT requires at least one projector");
    auto output_fields = outputFieldsOf(projectors);
    FieldSetNodePtr fields_out;

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
        auto group_key_span = spanOf(r.group_by->group_list);
        auto group_key =
            bindProjectors<bound::Projector>(std::move(r.group_by->group_list), bindProjector, ctx);

        for (auto&& p : group_key) {
            util::match(
                p,
                [&](bound::StarProjector&) {
                    throwAt(group_key_span, "Star projectors are not allowed in GROUP BY");
                },
                [](auto&&) {});
        }

        auto group_key_map = buildMap(group_key);

        for (auto&& p : projectors) {
            util::match(
                p,
                [](const bound::StarProjector&) { /* ok, all group keys */ },
                [&](const bound::IdentifierProjector& p) {
                    requireAt(
                        group_key_map.contains(p.field_id),
                        projectors_span,
                        "GROUP BY: unknown field {}",
                        to_string(p.field_id, *ctx.binding()));
                },
                [&](const bound::ExprProjector& p) {
                    if (p.expr->level != ExprKindLevel::Row) {
                        // Ok (Const/Group projectors)
                        return;
                    }
                    for (auto id : p.expr->required_fields.fieldIds()) {
                        requireAt(
                            group_key_map.contains(id),
                            projectors_span,
                            "GROUP BY: unknown field {}",
                            to_string(id, *ctx.binding()));
                    }
                });
        }

        group_by = bound::GroupBy{.group_list = std::move(group_key)};

        // Add group output keys
        generated_visible_fields->merge(outputFieldsOf(group_key));
        fields_out = FieldSetNode::make(output_fields);
    } else if (has_group_projector) {
        // Aggregate
        requireAt(!r.order_by, self.span, "ORDER BY does not make much sense with aggregates");

        for (auto&& p : projectors) {
            util::match(
                p,
                [&](const bound::StarProjector&) {
                    throwAt(projectors_span, "Star projectors are not allowed in aggregates");
                },
                [&](const bound::IdentifierProjector&) {
                    throwAt(projectors_span, "Identifier projectors are not allowed in aggregates");
                },
                [&](const bound::ExprProjector& p) {
                    requireAt(
                        p.expr->level != ExprKindLevel::Row,
                        projectors_span,
                        "Row projectors are not allowed in aggregates");
                });
        }
        fields_out = FieldSetNode::make(output_fields);
    } else {
        // Simple select, nothing left to check
        fields_out = FieldSetNode::make(output_fields, source_field_set_node);
    }

    std::optional<bound::OrderBy> order_by;
    if (r.order_by) {
        generated_visible_fields->merge(output_fields);

        auto order_list_span = spanOf(r.order_by->order_list);
        auto order_list = bindExprs(std::move(r.order_by->order_list), ctx);
        for (auto&& e : order_list) {
            requireAt(
                e.level != ExprKindLevel::Group,
                order_list_span,
                "ORDER BY cannot use aggregate expression here");
        }

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

bound::Relation bindRelation(ast::UnionAllRelation r, auto&& /*self*/, Context& ctx) {
    auto left = bindRelation(std::move(*r.left), ctx);
    auto right = bindRelation(std::move(*r.right), ctx);
    auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

    return {
        .node =
            bound::UnionAllRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .fields_out = std::move(fields),
    };
}

bound::Relation bindRelation(ast::UnionAllSortedByRelation r, auto&& self, Context& ctx) {
    auto left = bindRelation(std::move(*r.left), ctx);
    auto right = bindRelation(std::move(*r.right), ctx);
    auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

    FieldSetChain fields_set(fields, nullptr);
    auto _ = ctx.scopedFieldSet(&fields_set);

    auto order_list = bindExprs(std::move(r.order_by.order_list), ctx);
    requireAt(!order_list.empty(), self.span, "order list cannot be empty");

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

bound::Relation bindRelation(ast::FileRelation r, auto&& /*self*/, Context& /*ctx*/) {
    return {
        .node = bound::FileRelation{.path = std::move(r.path)},
        .fields_out = FieldSetNode::unknownSet(),
    };
}

bound::Relation bindRelation(ast::FileIntervalRelation r, auto&& /*self*/, Context& /*ctx*/) {
    constexpr auto format = TimeFormat::ISO8601;
    auto ts_from = timestampFromString(r.ts_from, format);

    return {
        .node =
            bound::FileIntervalRelation{
                .path = std::move(r.path),
                .ts_from = ts_from,
                .ts_to = ts_from + r.interval_s,
            },
        .fields_out = FieldSetNode::unknownSet(),
    };
}

bound::Relation bindRelation(ast::NamedRelationReferenceRelation r, auto&& self, Context& ctx) {
    auto child_node_ptr = ctx.find(r.name);
    requireAt(child_node_ptr != nullptr, self.span, "unknown named relation '{}'", r.name);

    return {
        .node = bound::NamedRelationReferenceRelation{.name = std::move(r.name)},
        .fields_out = FieldSetNode::proxy(child_node_ptr),
    };
}

bound::Relation bindRelation(ast::MaterializeRelation r, auto&& /*self*/, Context& ctx) {
    auto arg = bindRelation(std::move(*r.relation), ctx);
    auto fields = FieldSetNode::proxy(arg.fields_out);

    return {
        .node = bound::MaterializeRelation{.relation = box(std::move(arg))},
        .fields_out = std::move(fields),
    };
}

bound::Relation bindRelation(ast::Relation rel, Context& ctx) {
    return util::match(
        std::move(rel.node), [&](auto node) { return bindRelation(std::move(node), rel, ctx); });
}

}  // namespace lsql::front::sql::bind
