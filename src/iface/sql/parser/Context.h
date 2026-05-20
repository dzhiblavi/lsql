#pragma once

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"

namespace lsql::iface::sql::parse {

struct Context {
    ast::Program program;
    bool has_error = false;
};

}  // namespace lsql::iface::sql::parse
