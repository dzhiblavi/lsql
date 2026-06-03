#pragma once

#include "front/pipe/ast/Pipeline.h"
#include "front/pipe/bind/Context.h"
#include "front/pipe/bound/Pipeline.h"

namespace lsql::front::pipe::bind {

bound::Pipeline bindPipeline(ast::Pipeline st, Context& ctx);

}  // namespace lsql::front::pipe::bind
