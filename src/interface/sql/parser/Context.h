#pragma once

#include "interface/sql/ast/Node.h"

#include <memory>

namespace lsql::iface::sql::parse {

struct Context {
    std::unique_ptr<ast::Node> root;
    bool has_error = false;
};

}  // namespace lsql::iface::sql::parse
