#include "front/sql/bind/Statements.h"

#include "front/sql/bind/Relations.h"

#include "front/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "front/sql/ast/Relations.h"    // IWYU pragma: keep
#include "front/sql/ast/Statement.h"

#include "front/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "front/sql/bound/Statement.h"

namespace lsql::front::sql::bind {

bound::Statement bindStatement(ast::QueryStatement s, Context& ctx) {
    return bound::QueryStatement{
        .relation = box(bindRelation(std::move(*s.relation), ctx)),
    };
}

bound::Statement bindStatement(ast::NamedRelationStatement s, Context& ctx) {
    auto relation = box(bindRelation(std::move(*s.relation), ctx));
    ctx.insert(s.name, relation->fields_out);

    return bound::NamedRelationStatement{
        .name = s.name,
        .relation = std::move(relation),
    };
}

bound::Statement bindStatement(ast::Statement st, Context& ctx) {
    return util::match(std::move(st), [&](auto r) { return bindStatement(std::move(r), ctx); });
}

}  // namespace lsql::front::sql::bind
