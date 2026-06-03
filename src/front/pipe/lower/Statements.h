#pragma once

#include "front/pipe/bound/Statements.h"
#include "front/pipe/lower/Context.h"

#include "ir/Statement.h"

namespace lsql::front::pipe::lower {

ir::Statement lowerToIR(bound::Statement statement, Context& ctx);

ir::Program lowerToIR(bound::Program program);

}  // namespace lsql::front::pipe::lower
