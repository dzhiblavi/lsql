#include "front/pipe/lower/Statements.h"
#include "front/pipe/lower/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Pipeline.h"     // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep
#include "front/pipe/bound/Statements.h"   // IWYU pragma: keep

namespace lsql::front::pipe::lower {

namespace {

ir::Statement lowerToIR(bound::QueryStatement statement, Context& ctx) {
    auto pipeline_ctx = ctx.subContext();

    return ir::QueryStatement{
        .relation = box(lower::lowerToIR(std::move(*statement.pipeline), pipeline_ctx)),
    };
}

ir::Statement lowerToIR(bound::NamedPipelineStatement statement, Context& ctx) {
    auto pipeline_ctx = ctx.subContext();
    auto relation = box(lower::lowerToIR(std::move(*statement.pipeline), pipeline_ctx));
    ctx.insert(statement.name, relation->schema);

    return ir::NamedRelationStatement{
        .name = std::move(statement.name),
        .relation = std::move(relation),
    };
}

}  // namespace

ir::Statement lowerToIR(bound::Statement statement, Context& ctx) {
    return util::match(std::move(statement), [&](auto s) { return lowerToIR(std::move(s), ctx); });
}

ir::Program lowerToIR(bound::Program program) {
    ContextBase base(program.binding);
    Context ctx(&base);

    auto p = ir::Program{
        .statements = {},
        .field_binding = program.binding,
    };

    for (auto&& s : program.statements) {
        p.statements.push_back(lowerToIR(std::move(s), ctx));
    }

    return p;
}

}  // namespace lsql::front::pipe::lower
