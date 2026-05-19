#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Source.h"

#include <vector>

namespace lsql::iface {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<exec::OperationPtr> top_operations;
};

class Interface {
 public:
    virtual ~Interface() = default;

    virtual Plan plan() const = 0;
};

}  // namespace lsql::iface
