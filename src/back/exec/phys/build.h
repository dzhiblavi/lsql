#pragma once

#include "back/exec/phys/Source.h"
#include "back/exec/plan/plan.h"

#include <vector>

namespace lsql::back::exec::phys {

struct Output {
    Schema schema;
    Arc<Operation> operation;
};

struct Phase {
    std::vector<Arc<Source>> sources;
    std::vector<Output> outputs;
};

struct Program {
    std::vector<Arc<const void>> anchors;
    std::map<int, Phase> phases;
};

Program build(const plan::Plan& plan);

}  // namespace lsql::back::exec::phys
