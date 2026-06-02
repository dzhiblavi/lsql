#pragma once

#include <cstddef>

namespace lsql::front::pipe::parse {

struct Token {
    int code;
    const char* text;
};

}  // namespace lsql::front::pipe::parse
