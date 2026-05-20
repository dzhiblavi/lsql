#pragma once

#include "core/ValueType.h"

#include <string>

namespace lsql::iface::sql::ast {

struct Literal {
    ValueType type;
    std::string value_str;
};

}  // namespace lsql::iface::sql
