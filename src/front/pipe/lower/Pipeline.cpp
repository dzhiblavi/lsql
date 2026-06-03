#include "front/pipe/lower/Pipeline.h"
#include "front/pipe/lower/Sources.h"
#include "front/pipe/lower/Stages.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"

#include "front/pipe/lower/Context.h"
#include "front/pipe/lower/Pipeline.h"

namespace lsql::front::pipe::lower {

ir::Relation lowerToIR(bound::Pipeline pipe, Context& ctx) {
    verify(!ctx.hasRelation(), "will damage context");
    ctx.setRelation(lowerToIR(std::move(*pipe.source), ctx));

    for (auto&& stage : pipe.stages) {
        auto _ = ctx.scopedFieldSet(&ctx.currRelation().fields_out);
        ctx.setRelation(lowerToIR(std::move(stage), ctx));
    }

    return ctx.pullRelation();
}

}  // namespace lsql::front::pipe::lower
