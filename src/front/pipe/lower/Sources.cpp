#include "front/pipe/lower/Sources.h"
#include "front/pipe/lower/Expressions.h"
#include "front/pipe/lower/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep

namespace lsql::front::pipe::lower {

namespace {

ir::Relation lowerToIR(bound::AdhocSource s, auto& self, Context& /*ctx*/) {
    return {
        .node =
            ir::ValuesRelation{
                .values = std::move(s.values),
                .output_id = s.output_field_id,
            },
        .fields_out = self.fields_out->fieldSet(),
    };
}

ir::Relation lowerToIR(bound::NamedPipelineReferenceSource s, auto& /*self*/, Context& ctx) {
    return {
        .node = ir::NamedRelationReferenceRelation{.name = s.name},
        .fields_out = ctx.find(s.name),
    };
}

ir::Relation lowerToIR(bound::FileSource s, auto& self, Context& ctx) {
    auto fields = self.fields_out->fieldSet();
    llog::info("path={}, requested fields: {}", s.path, to_string(fields, *ctx.binding()));

    return {
        .node = ir::FileRelation{.path = std::move(s.path)},
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::FileIntervalSource s, auto& self, Context& ctx) {
    auto fields = self.fields_out->fieldSet();
    llog::info(
        "(interval) path={}, requested fields: {}", s.path, to_string(fields, *ctx.binding()));

    return {
        .node =
            ir::FileIntervalRelation{
                .path = std::move(s.path),
                .ts_from = s.ts_from,
                .ts_to = s.ts_to,
            },
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::UnionAllSource s, auto& /*self*/, Context& ctx) {
    auto left_ctx = ctx.subContext();
    auto left = lower::lowerToIR(std::move(*s.left), left_ctx);
    auto right_ctx = ctx.subContext();
    auto right = lower::lowerToIR(std::move(*s.right), right_ctx);
    auto fields = FieldSet::merge(left.fields_out, right.fields_out);

    return {
        .node =
            ir::UnionAllRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::UnionAllSortedBySource s, auto& /*self*/, Context& ctx) {
    auto left_ctx = ctx.subContext();
    auto left = lower::lowerToIR(std::move(*s.left), left_ctx);
    auto right_ctx = ctx.subContext();
    auto right = lower::lowerToIR(std::move(*s.right), right_ctx);
    auto fields = FieldSet::merge(left.fields_out, right.fields_out);

    auto _ = ctx.scopedFieldSet(&fields);
    auto [order_list, aggregates] = lowerToIR(std::move(s.order_list), ctx);
    verify(aggregates.empty());

    return {
        .node =
            ir::UnionAllSortedByRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
                .order_list = std::move(order_list),
                .desc = s.desc,
            },
        .fields_out = fields,
    };
}

}  // namespace

ir::Relation lowerToIR(bound::Source s, Context& ctx) {
    return util::match(std::move(s.node), [&](auto r) { return lowerToIR(std::move(r), s, ctx); });
}

}  // namespace lsql::front::pipe::lower
