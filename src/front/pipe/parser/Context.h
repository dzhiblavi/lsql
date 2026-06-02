#pragma once

#include "front/pipe/ast/Pipeline.h"

namespace lsql::front::pipe::parse {

struct Context {
    ast::Pipeline pipeline;
    bool has_error = false;
};

}  // namespace lsql::front::pipe::parse
