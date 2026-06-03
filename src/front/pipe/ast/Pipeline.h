#pragma once

#include "front/pipe/ast/fwd/Source.h"
#include "front/pipe/ast/fwd/Stage.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::ast {

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
};

}  // namespace lsql::front::pipe::ast
