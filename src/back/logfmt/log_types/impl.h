#pragma once

#include "back/logfmt/LogType.h"

#include <cstddef>

namespace lsql::back::logfmt {

static constexpr size_t ExpectedKeysCountTunable = 32;

template <LogType Type>
struct LogTypeImpl;

}  // namespace lsql::back::logfmt
