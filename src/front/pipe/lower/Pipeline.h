#pragma once

#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/lower/Context.h"

#include "ir/Relation.h"

namespace lsql::front::pipe::lower {

ir::Relation lowerToIR(bound::Pipeline pipeline, Context& ctx);

}  // namespace lsql::front::pipe::lower
