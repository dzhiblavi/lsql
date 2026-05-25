#include "iface/sql/lower/lower.h"

#include "iface/sql/lower/Context.h"
#include "iface/sql/lower/Statements.h"

#include "iface/sql/bound/Expressions.h"  // IWYU pragma: keep

namespace lsql::iface::sql::lower {

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

}  // namespace lsql::iface::sql::lower
