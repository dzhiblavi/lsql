#pragma once

#include "front/pipe/ast/Pipeline.h"

#include <vector>

namespace lsql::front::pipe::ast {

struct QueryStatement {
    Box<Pipeline> pipeline;
};

struct NamedPipelineStatement {
    std::string name;
    Box<Pipeline> pipeline;
};

using Statement = std::variant< //
    QueryStatement, //
    NamedPipelineStatement //
>;

struct Program {
    std::vector<Statement> statements;
};

}  // namespace lsql::front::pipe::ast
