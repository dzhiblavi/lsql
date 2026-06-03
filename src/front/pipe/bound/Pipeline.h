#pragma once

#include "front/pipe/bound/fwd/Source.h"  // IWYU pragma: keep
#include "front/pipe/bound/fwd/Stage.h"   // IWYU pragma: keep

#include "front/FieldSetNode.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::bound {

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
    FieldSetNodePtr fields_out;
};

}  // namespace lsql::front::pipe::bound
