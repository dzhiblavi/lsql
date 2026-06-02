#pragma once

#include "core/ValueType.h"

#include <string>

namespace lsql::front::pipe::ast {

struct Literal {
    ValueType type;
    std::string value_str;
};

}  // namespace lsql::front::pipe::ast
