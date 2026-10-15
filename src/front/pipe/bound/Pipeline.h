#pragma once

#include "front/pipe/bound/fwd/Source.h"  // IWYU pragma: keep
#include "front/pipe/bound/fwd/Stage.h"   // IWYU pragma: keep

#include "front/common/bound/RelationFieldSet.h"
#include "front/common/source/SourceSpan.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::bound {

using common::bound::RelationFieldSetPtr;

struct Pipeline {
    Box<Source> source;
    std::vector<Stage> stages;
    RelationFieldSetPtr fields_out;
    SourceSpan span;
};

}  // namespace lsql::front::pipe::bound
