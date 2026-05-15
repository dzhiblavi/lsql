#pragma once

#include "logs/LogType.h"

#include <cstddef>

namespace lsql::logs {

static constexpr size_t ExpectedKeysCountTunable = 32;

template <LogType Type>
struct LogTypeImpl;

}  // namespace lsql::logs
