#pragma once

#include "sql/ast/Node.h"

#include <memory>

namespace lsql::sql::parse {

struct Context {
    std::unique_ptr<ast::Node> root;
    bool has_error = false;
};

}  // namespace lsql::sql::parse
