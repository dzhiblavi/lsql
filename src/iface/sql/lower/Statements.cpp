#include "iface/sql/lower/Statements.h"

#include "iface/sql/lower/Relations.h"

#include "iface/sql/bound/Expressions.h"  // IWYU pragma: keep

namespace lsql::iface::sql::lower {

namespace {

ir::Statement lowerToIR(bound::QueryStatement s, Context& ctx) {
    auto r = lowerToIR(std::move(*s.relation), ctx);
    return ir::QueryStatement{.relation = box(std::move(r))};
}

ir::Statement lowerToIR(bound::NamedRelationStatement s, Context& ctx) {
    auto relation = box(lowerToIR(std::move(*s.relation), ctx));
    ctx.insertRelation(s.name, relation.get());

    return ir::NamedRelationStatement{
        .name = s.name,
        .relation = std::move(relation),
    };
}

}  // namespace

ir::Statement lowerToIR(bound::Statement st, Context& ctx) {
    return util::match(std::move(st), [&](auto r) { return lowerToIR(std::move(r), ctx); });
}

}  // namespace lsql::iface::sql::lower
