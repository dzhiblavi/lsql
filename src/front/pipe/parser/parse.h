#pragma once

#include "front/pipe/ast/Statements.h"

#include <string>

namespace lsql::front::pipe::parse {

ast::Program parse(std::string query);

}  // namespace lsql::front::pipe::parse
