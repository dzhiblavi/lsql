#include "front/pipe/bind/Statements.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/bind/Pipeline.h"

namespace lsql::front::pipe::bind {

namespace {

bound::Statement bindStatement(ast::QueryStatement s, Context& ctx) {
    return bound::QueryStatement{
        .pipeline = box(bindPipeline(std::move(*s.pipeline), ctx)),
    };
}

bound::Statement bindStatement(ast::NamedPipelineStatement s, Context& ctx) {
    auto pipeline = box(bindPipeline(std::move(*s.pipeline), ctx));
    ctx.insertPipeline(s.name, pipeline->fields_out);

    return bound::NamedPipelineStatement{
        .name = std::move(s.name),
        .pipeline = std::move(pipeline),
    };
}

}  // namespace

bound::Statement bindStatement(ast::Statement s, Context& ctx) {
    return util::match(std::move(s), [&](auto s) { return bindStatement(std::move(s), ctx); });
}

bound::Program bindProgram(ast::Program p) {
    Context ctx(arc<FieldBinding>());

    auto prog = bound::Program{
        .statements = {},
        .binding = ctx.binding(),
    };

    for (auto&& s : p.statements) {
        prog.statements.push_back(bindStatement(std::move(s), ctx));
    }

    return prog;
}

}  // namespace lsql::front::pipe::bind
