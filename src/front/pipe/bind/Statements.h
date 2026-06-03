#pragma once

#include "front/pipe/ast/Statements.h"
#include "front/pipe/bind/Context.h"
#include "front/pipe/bound/Statements.h"

namespace lsql::front::pipe::bind {

bound::Statement bindStatement(ast::Statement s, Context& ctx);

bound::Program bindProgram(ast::Program p);

}  // namespace lsql::front::pipe::bind
