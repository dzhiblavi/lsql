#pragma once

#include "ir/Aggregates.h"
#include "ir/Relations.h"

#include "core/Fields.h"

namespace lsql::ir {

FieldSet outputFieldsOf(const std::vector<Projector>& ps);
FieldSet outputFieldsOf(const std::vector<Aggregate>& ps);

}  // namespace lsql::ir
