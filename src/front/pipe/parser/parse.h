#pragma once

#include "front/pipe/ast/Statements.h"

#include <istream>

namespace lsql::front::pipe::parse {

ast::Program parse(std::istream& is);

}  // namespace lsql::front::pipe::parse
