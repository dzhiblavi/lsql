#include "front/sql/lower/lower.h"

#include "front/sql/lower/Context.h"
#include "front/sql/lower/Statements.h"

#include "front/sql/bound/Expressions.h"  // IWYU pragma: keep

namespace lsql::front::sql::lower {

ir::Program lowerToIR(bound::Program program) {
    ir::Program p{
        .statements = {},
        .field_binding = program.binding,
    };

    Context ctx{program.binding};

    for (auto&& statement : program.statements) {
        p.statements.push_back(lowerToIR(std::move(statement), ctx));
    }

    return p;
}

}  // namespace lsql::front::sql::lower
