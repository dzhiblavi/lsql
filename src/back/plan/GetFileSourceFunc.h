#pragma once

#include "back/exec/op/Source.h"
#include "back/plan/TimeRange.h"

#include "core/Fields.h"

#include <functional>
#include <string>

namespace lsql::back::plan {

using GetFileSourceFuncType = std::function<back::exec::SourcePtr(
    std::string, ConstFieldBindingPtr, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

}  // namespace lsql::back::plan
