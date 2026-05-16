#pragma once

#include <cstddef>

namespace lsql::sql::parse {

struct Token {
    int code;
    const char* text;
};

}  // namespace lsql::sql::parse
