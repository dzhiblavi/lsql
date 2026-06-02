#pragma once

#include "front/pipe/ast/Pipeline.h"

#include <istream>

namespace lsql::front::pipe::parse {

ast::Pipeline parse(std::istream& is);

}  // namespace lsql::front::pipe::parse
