#include "front/pipe/bind/Statements.h"

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep

#include "front/pipe/bind/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep

#include "front/common/source/require_at.h"

namespace lsql::front::pipe::bind {

namespace {

bound::Statement bindStatement(ast::QueryStatement s, auto&& /*self*/, Context& ctx) {
    return bound::QueryStatement{
        .pipeline = box(bindPipeline(std::move(*s.pipeline), ctx)),
    };
}

bound::Statement bindStatement(ast::NamedPipelineStatement s, auto&& self, Context& ctx) {
    auto pipeline = box(bindPipeline(std::move(*s.pipeline), ctx));
    requireAt(
        ctx.insert(s.name, pipeline->fields_out),
        self.span,
        "duplicate named relation '{}'",
        s.name);

    return bound::NamedPipelineStatement{
        .name = std::move(s.name),
        .pipeline = std::move(pipeline),
    };
}

}  // namespace

bound::Statement bindStatement(ast::Statement s, Context& ctx) {
    return util::match(
        std::move(s.node), [&](auto node) { return bindStatement(std::move(node), s, ctx); });
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
