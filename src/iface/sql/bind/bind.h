#pragma once

#include "iface/sql/bind/Statement.h"

#include "iface/sql/ast/Statement.h"

#include "core/Fields.h"

namespace lsql::iface::sql::bind {

struct BoundProgram {
    Program program;
    ConstFieldBindingPtr field_binding;
};

BoundProgram bind(ast::Program program);

}  // namespace lsql::iface::sql::bind
