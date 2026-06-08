#pragma once

#include "front/common/source/SourceSpan.h"

#include <cstddef>

namespace lsql::front::pipe::parse {

struct Token {  // NOLINT
    int code;
    const char* text;
    SourceSpan span;
};

}  // namespace lsql::front::pipe::parse
