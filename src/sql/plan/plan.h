#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Source.h"

#include "sql/ast/Node.h"
#include "sql/plan/FileSourceFunc.h"

#include <vector>

namespace lsql::sql::plan {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<exec::OperationPtr> top_operations;
};

Plan plan(const ast::Node& root, GetFileSourceFuncType get_file_source = defaultFileSourceFunc());

}  // namespace lsql::sql::plan
