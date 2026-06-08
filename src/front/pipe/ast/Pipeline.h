#pragma once

#include "front/pipe/ast/fwd/Source.h"
#include "front/pipe/ast/fwd/Stage.h"

#include "front/common/source/SourceSpan.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::ast {

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
    SourceSpan span;
};

}  // namespace lsql::front::pipe::ast
