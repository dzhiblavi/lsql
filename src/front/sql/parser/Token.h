#pragma once

#include <cstddef>

namespace lsql::front::sql::parse {

struct Token {
    int code;
    const char* text;
};

}  // namespace lsql::front::sql::parse
