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
#include "util/archive.h"

#include <ranges>

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

bound::Relation bindRelation(ast::SelectRelation r, auto&& /*self*/, Context& ctx) {
    auto source = bindRelation(std::move(*r.source), ctx);
    auto source_visible_fields = FieldSetChain(source.fields_out, nullptr);

    std::optional<bound::Where> where;
    if (r.where) {
        // Where sees only source fields
        auto _ = ctx.scopedFieldSet(&source_visible_fields);
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
        requireAt(r.limit->limit >= 0, r.limit->span, "limit cannot be negative");
        limit = bound::Limit{.limit = r.limit->limit};
    }

    auto projectors_spans = spansOf(r.projectors);
    auto projectors_span = spanOf(r.projectors);

    // The following code should fill these
    FieldSetNodePtr fields_out = nullptr;
    FieldSetNodePtr order_by_visible_fields = nullptr;
    std::vector<bound::Projector> projectors;
    bool aggregate = false;

    std::optional<bound::GroupBy> group_by;
    if (r.group_by.has_value()) {
        auto group_key_spans = spansOf(r.group_by->group_list);

        auto group_key = [&] {
            // Group by keys see source fields only
            auto _ = ctx.scopedFieldSet(&source_visible_fields);
            return bindProjectors<bound::Projector>(
                std::move(r.group_by->group_list), bindProjector, ctx);
        }();

        for (auto&& [p, span] : std::views::zip(group_key, group_key_spans)) {
            util::match(
                p,
                [&](bound::StarProjector&) {
                    throwAt(span, "star projectors are not allowed in GROUP BY");
                },
                [](auto&&) {});
        }
        auto group_key_map = buildMap(group_key);

        projectors = [&] {
            // Projectors see group keys and source visible fields
            auto group_key_fields = FieldSetNode::make(outputFieldsOf(group_key));
            auto proj_visible_fields = FieldSetChain(group_key_fields, &source_visible_fields);
            auto _ = ctx.scopedFieldSet(&proj_visible_fields);

            return bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
        }();
        requireAt(!projectors.empty(), projectors_span, "SELECT requires at least one projector");

        for (auto&& [p, span] : std::views::zip(projectors, projectors_spans)) {
            util::match(
                p,
                [](const bound::StarProjector&) { /* ok, all group keys */ },
                [&](const bound::IdentifierProjector& p) {
                    requireAt(
                        group_key_map.contains(p.field_id),
                        span,
                        "GROUP BY: unknown identifier {}",
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
                            span,
                            "GROUP BY: unknown identifier {}",
                            to_string(id, *ctx.binding()));
                    }
                });
        }

        // Projectors' and group key fields are visible to order by
        order_by_visible_fields = FieldSetNode::make(
            FieldSet::merge(outputFieldsOf(projectors), outputFieldsOf(group_key)));

        // Only projectors' fields are in output
        fields_out = FieldSetNode::make(outputFieldsOf(projectors));

        group_by = bound::GroupBy{.group_list = std::move(group_key)};
        aggregate = true;
    } else {
        // Simple SELECT or aggregate SELECT
        projectors = [&] {
            // Projectors see source visible fields only
            auto _ = ctx.scopedFieldSet(&source_visible_fields);
            return bindProjectors<bound::Projector>(std::move(r.projectors), bindProjector, ctx);
        }();
        requireAt(!projectors.empty(), projectors_span, "SELECT requires at least one projector");

        bool has_group_projector = false;
        bool has_star_projector = false;
        for (auto&& p : projectors) {
            util::match(
                p,
                [&](const bound::ExprProjector& p) {
                    has_group_projector |= p.expr->level == ExprKindLevel::Group;
                },
                [&](const bound::IdentifierProjector&) {},
                [&](const bound::StarProjector&) { has_star_projector = true; });
        }

        if (has_group_projector) {
            // Aggregate SELECT
            aggregate = true;

            for (auto&& [p, span] : std::views::zip(projectors, projectors_spans)) {
                util::match(
                    p,
                    [&](const bound::StarProjector&) {
                        throwAt(span, "star projectors are not allowed in aggregates");
                    },
                    [&](const bound::IdentifierProjector&) {
                        throwAt(span, "identifier projectors are not allowed in aggregates");
                    },
                    [&](const bound::ExprProjector& p) {
                        requireAt(
                            p.expr->level != ExprKindLevel::Row,
                            span,
                            "row projectors are not allowed in aggregates");
                    });
            }

            // Order by sees projectors' fields only
            order_by_visible_fields = FieldSetNode::make(outputFieldsOf(projectors));
            // Only projectors' fields are in output
            fields_out = FieldSetNode::make(outputFieldsOf(projectors));
        } else {
            // Simple SELECT

            // Order by sees both projectors and source's visible fields
            order_by_visible_fields =
                FieldSetNode::make(outputFieldsOf(projectors), source.fields_out);

            if (has_star_projector) {
                // Both source and projectors' fields are in output (because * is present)
                fields_out = FieldSetNode::make(outputFieldsOf(projectors), source.fields_out);
            } else {
                // Only projectors' fields are in output (no * in select list)
                fields_out = FieldSetNode::make(outputFieldsOf(projectors));
            }
        }
    }

    std::optional<bound::OrderBy> order_by;
    if (r.order_by) {
        auto order_list_spans = spansOf(r.order_by->order_list);

        auto order_list = [&] {
            auto visible_fields = FieldSetChain(order_by_visible_fields, nullptr);
            auto _ = ctx.scopedFieldSet(&visible_fields);

            return bindExprs(std::move(r.order_by->order_list), ctx);
        }();

        for (auto&& [e, span] : std::views::zip(order_list, order_list_spans)) {
            requireAt(
                e.level != ExprKindLevel::Group,
                span,
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
                .aggregate = aggregate,
            },
        .fields_out = fields_out,
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

bound::Relation bindRelation(ast::FileIntervalRelation r, auto&& self, Context& /*ctx*/) {
    requireAt(
        !util::isProbablyArchive(r.path),
        self.span,
        "time intervals cannot be applied to archives");

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
