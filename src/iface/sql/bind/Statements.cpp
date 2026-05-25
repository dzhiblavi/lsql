#include "iface/sql/bind/Statements.h"

#include "iface/sql/bind/Relations.h"

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"

#include "iface/sql/bound/Statement.h"

namespace lsql::iface::sql::bind {

bound::Statement bindStatement(ast::QueryStatement s, Context& ctx) {
    return bound::QueryStatement{
        .relation = box(bindRelation(std::move(*s.relation), ctx)),
    };
}

bound::Statement bindStatement(ast::NamedRelationStatement s, Context& ctx) {
    auto relation = box(bindRelation(std::move(*s.relation), ctx));
    ctx.insertRelation(s.name, relation.get());

    return bound::NamedRelationStatement{
        .name = s.name,
        .relation = std::move(relation),
    };
}

bound::Statement bindStatement(ast::Statement st, Context& ctx) {
    return util::match(std::move(st), [&](auto r) { return bindStatement(std::move(r), ctx); });
}

}  // namespace lsql::iface::sql::bind
