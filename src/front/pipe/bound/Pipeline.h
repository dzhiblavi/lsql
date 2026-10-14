#pragma once

#include "front/pipe/bound/fwd/Source.h"  // IWYU pragma: keep
#include "front/pipe/bound/fwd/Stage.h"   // IWYU pragma: keep

#include "front/common/bound/RelationFieldSet.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::bound {

using common::bound::RelationFieldSetPtr;

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
    RelationFieldSetPtr fields_out;
};

}  // namespace lsql::front::pipe::bound
