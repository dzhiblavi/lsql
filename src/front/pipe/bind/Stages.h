#pragma once

#include "front/pipe/bind/Context.h"

#include "front/pipe/ast/fwd/Stage.h"
#include "front/pipe/bound/fwd/Stage.h"

namespace lsql::front::pipe::bind {

bound::Stage bindStage(ast::Stage rel, Context& ctx);

}  // namespace lsql::front::pipe::bind
