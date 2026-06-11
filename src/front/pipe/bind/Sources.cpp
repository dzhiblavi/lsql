#include "front/pipe/bind/Sources.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/bind/Expressions.h"
#include "front/pipe/bind/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep

#include "front/common/source/require_at.h"

#include "core/time_formats.h"
#include "util/archive.h"

namespace lsql::front::pipe::bind {

namespace {

using common::bind::FieldSetChain;
using common::bound::FieldSetNode;

bound::Source bindSource(ast::AdhocSource s, auto&& self, Context& ctx) {
    std::vector<Value> values;
    values.reserve(s.literals.size());
    for (auto&& literal : s.literals) {
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
            bound::AdhocSource{
                .values = std::move(values),
                .output_field_id = id,
            },
        .fields_out = FieldSetNode::make(FieldSet::withField(id)),
    };
}

bound::Source bindSource(ast::NamedPipelineReferenceSource s, auto&& self, Context& ctx) {
    auto child_node_ptr = ctx.find(s.name);
    requireAt(child_node_ptr != nullptr, self.span, "unknown named pipeline '{}'", s.name);

    return {
        .node = bound::NamedPipelineReferenceSource{.name = s.name},
        .fields_out = FieldSetNode::proxy(child_node_ptr),
    };
}

bound::Source bindSource(ast::FileSource s, auto&& /*self*/, Context& /*ctx*/) {
    return {
        .node = bound::FileSource{.path = std::move(s.path)},
        .fields_out = FieldSetNode::unknownSet(),
    };
}

bound::Source bindSource(ast::FileIntervalSource s, auto&& self, Context& /*ctx*/) {
    requireAt(
        !util::isProbablyArchive(s.path),
        self.span,
        "time intervals cannot be applied to archives");

    constexpr auto format = TimeFormat::ISO8601;
    auto ts_from = timestampFromString(s.ts_from, format);

    return {
        .node =
            bound::FileIntervalSource{
                .path = std::move(s.path),
                .ts_from = ts_from,
                .ts_to = ts_from + s.interval_s,
            },
        .fields_out = FieldSetNode::unknownSet(),
    };
}

bound::Source bindSource(ast::UnionAllSource s, auto&& /*self*/, Context& ctx) {
    auto left = bindPipeline(std::move(*s.left), ctx);
    auto right = bindPipeline(std::move(*s.right), ctx);
    auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

    return {
        .node =
            bound::UnionAllSource{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .fields_out = std::move(fields),
    };
}

bound::Source bindSource(ast::UnionAllSortedBySource s, auto&& self, Context& ctx) {
    auto left = bindPipeline(std::move(*s.left), ctx);
    auto right = bindPipeline(std::move(*s.right), ctx);
    auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

    FieldSetChain visible_fields(fields, nullptr);
    auto _ = ctx.scopedFieldSet(&visible_fields);

    auto order_list = bindExprs(std::move(s.order_list), ctx);
    requireAt(!order_list.empty(), self.span, "order list cannot be empty");

    return {
        .node =
            bound::UnionAllSortedBySource{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
                .order_list = std::move(order_list),
                .desc = s.desc,
            },
        .fields_out = std::move(fields),
    };
}

}  // namespace

bound::Source bindSource(ast::Source s, Context& ctx) {
    return util::match(
        std::move(s.node), [&](auto node) { return bindSource(std::move(node), s, ctx); });
}

}  // namespace lsql::front::pipe::bind
