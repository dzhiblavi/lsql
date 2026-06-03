#pragma once

#include "front/pipe/bound/Pipeline.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::bound {

struct QueryStatement {
    Box<Pipeline> pipeline;
};

struct NamedPipelineStatement {
    std::string name;
    Box<Pipeline> pipeline;
};

using Statement = std::variant< //
    QueryStatement,         //
    NamedPipelineStatement  //
>;

struct Program {
    std::vector<Statement> statements;
    FieldBindingPtr binding;
};

}  // namespace lsql::front::pipe::bound
