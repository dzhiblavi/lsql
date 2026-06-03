#pragma once

#include "ir/Relation.h"

#include "front/pipe/bound/Sources.h"
#include "front/pipe/lower/Context.h"

namespace lsql::front::pipe::lower {

ir::Relation lowerToIR(bound::Source st, Context& ctx);

}  // namespace lsql::front::pipe::lower
