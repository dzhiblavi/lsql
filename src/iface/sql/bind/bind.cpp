#include "iface/sql/bind/bind.h"

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep

#include "iface/sql/bind/Statements.h"

namespace lsql::iface::sql::bind {

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

}  // namespace lsql::iface::sql::bind
