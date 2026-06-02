#pragma once

#include "front/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "front/sql/ast/Relations.h"    // IWYU pragma: keep
#include "front/sql/ast/Statement.h"

namespace lsql::front::sql::parse {

struct Context {
    ast::Program program;
    bool has_error = false;
};

}  // namespace lsql::front::sql::parse
