#include "front/sql/lower/Statements.h"

#include "front/sql/lower/Relations.h"

#include "front/sql/bound/Expressions.h"  // IWYU pragma: keep

namespace lsql::front::sql::lower {

namespace {

ir::Statement lowerToIR(bound::QueryStatement s, Context& ctx) {
    auto r = lowerToIR(std::move(*s.relation), ctx);
    return ir::QueryStatement{.relation = box(std::move(r))};
}

ir::Statement lowerToIR(bound::NamedRelationStatement s, Context& ctx) {
    auto relation = box(lowerToIR(std::move(*s.relation), ctx));
    ctx.insert(s.name, relation->fields_out);

    return ir::NamedRelationStatement{
        .name = s.name,
        .relation = std::move(relation),
    };
}

}  // namespace

ir::Statement lowerToIR(bound::Statement st, Context& ctx) {
    return util::match(std::move(st), [&](auto r) { return lowerToIR(std::move(r), ctx); });
}

}  // namespace lsql::front::sql::lower
