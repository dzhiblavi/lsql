#pragma once

#include "core/value/Value.h"

namespace lsql {

std::optional<Value> cast(Value val, ValueType to);

}  // namespace lsql
