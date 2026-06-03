#pragma once

#include "front/pipe/bind/Context.h"

#include "front/pipe/ast/fwd/Source.h"
#include "front/pipe/bound/fwd/Source.h"

namespace lsql::front::pipe::bind {

bound::Source bindSource(ast::Source rel, Context& ctx);

}  // namespace lsql::front::pipe::bind
