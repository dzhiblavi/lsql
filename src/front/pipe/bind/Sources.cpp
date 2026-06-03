#include "front/pipe/bind/Sources.h"

#include "core/time_formats.h"
#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/bind/Expressions.h"
#include "front/pipe/bind/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep

namespace lsql::front::pipe::bind {

namespace {

bound::Source bindSource(ast::AdhocSource s, Context& ctx) {
    std::vector<Value> values;
    values.reserve(s.literals.size());
    for (auto&& literal : s.literals) {
        values.push_back(parseLiteral(literal));
        require(
            values.back().type() == values.front().type(),
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

bound::Source bindSource(ast::NamedPipelineReferenceSource s, Context& ctx) {
    return {
        .node = bound::NamedPipelineReferenceSource{.name = s.name},
        .fields_out = FieldSetNode::proxy(ctx.findPipeline(s.name)),
    };
}

bound::Source bindSource(ast::FileSource s, Context& /*ctx*/) {
    return {
        .node = bound::FileSource{.path = std::move(s.path)},
        .fields_out = FieldSetNode::unknownSet(),
    };
}

bound::Source bindSource(ast::FileIntervalSource s, Context& /*ctx*/) {
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

bound::Source bindSource(ast::UnionAllSource s, Context& ctx) {
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

bound::Source bindSource(ast::UnionAllSortedBySource s, Context& ctx) {
    auto left = bindPipeline(std::move(*s.left), ctx);
    auto right = bindPipeline(std::move(*s.right), ctx);
    auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

    FieldSetChain visible_fields(fields, nullptr);
    auto _ = ctx.scopedFieldSet(&visible_fields);

    auto order_list = bindExprs(std::move(s.order_list), ctx);
    require(!order_list.empty(), "order list cannot be empty");

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
    return util::match(std::move(s), [&](auto r) { return bindSource(std::move(r), ctx); });
}

}  // namespace lsql::front::pipe::bind
