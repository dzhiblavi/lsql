#pragma once

#include "ir/Aggregates.h"
#include "ir/Relations.h"

#include "core/schema/Schema.h"

namespace lsql::ir {

Schema schemaFor(const std::vector<ir::Projector>& ps);
Schema schemaFor(const std::vector<ir::Aggregate>& ps);

}  // namespace lsql::ir
