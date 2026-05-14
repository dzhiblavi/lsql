#pragma once

#include "sql/ast/Node.h"

#include <memory>

namespace lsql::sql::parse {

struct Context {
    std::unique_ptr<ast::Node> root;
    int has_error;
};

}  // namespace lsql::sql::parse
