#include "front/pipe/bind/Pipeline.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/bind/Sources.h"
#include "front/pipe/bind/Stages.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/Sources.h"
#include "front/pipe/bound/Stages.h"

namespace lsql::front::pipe::bind {

bound::Pipeline bindPipeline(ast::Pipeline st, Context& ctx) {
    auto source = box([&] {
        // Clear current field set so nothing leaks from outside
        auto _ = ctx.scopedFieldSet(nullptr);
        return bindSource(std::move(*st.source), ctx);
    }());

    bound::Pipeline p{
        .source = std::move(source),
        .stages = {},
        .fields_out = nullptr,
    };

    auto prev_fields_out = p.source->fields_out;

    for (auto&& stage : st.stages) {
        auto visible_fields = FieldSetChain(prev_fields_out, nullptr);
        auto _ = ctx.scopedFieldSet(&visible_fields);
        auto bound_stage = bindStage(std::move(stage), ctx);

        prev_fields_out = bound_stage.fields_out;
        p.stages.push_back(std::move(bound_stage));
    }

    p.fields_out = FieldSetNode::proxy(prev_fields_out);
    return p;
}

}  // namespace lsql::front::pipe::bind
