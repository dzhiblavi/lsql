#pragma once

#include "front/pipe/ast/Statements.h"

namespace lsql::front::pipe::parse {

struct Context {
    ast::Program program;
    bool has_error = false;
};

}  // namespace lsql::front::pipe::parse
