#pragma once

#include "front/pipe/ast/fwd/Source.h"  // IWYU pragma: keep
#include "front/pipe/ast/fwd/Stage.h"   // IWYU pragma: keep

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::ast {

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
};

}  // namespace lsql::front::pipe::ast
