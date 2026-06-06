#include "front/sql/bind/bind.h"

#include "front/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "front/sql/ast/Relations.h"    // IWYU pragma: keep

#include "front/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "front/sql/bound/Relations.h"    // IWYU pragma: keep

#include "front/sql/bind/Statements.h"

namespace lsql::front::sql::bind {

bound::Program bind(ast::Program program) {
    bound::Program p{
        .statements = {},
        .binding = arc<FieldBinding>(),
    };

    Context ctx{p.binding};

    for (auto&& statement : program) {
        p.statements.push_back(bindStatement(std::move(statement), ctx));
    }

    return p;
}

}  // namespace lsql::front::sql::bind
