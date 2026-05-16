#include "sql/plan/plan.h"

#include "sql/plan/ExecVisitor.h"

namespace lsql::sql::plan {

Plan plan(const ast::Node& root, GetFileSourceFuncType get_file_source) {
    ExecVisitor exec_visitor(get_file_source);
    root.visit(exec_visitor);

    auto [sources, operations] = std::move(exec_visitor).result();

    Plan plan;
    plan.sources = std::move(sources);
    plan.top_operations.reserve(operations.size());

    while (!operations.empty()) {
        plan.top_operations.push_back(std::move(operations.top()));
        operations.pop();
    }

    std::ranges::reverse(plan.top_operations);
    return plan;
}

}  // namespace lsql::sql::plan
