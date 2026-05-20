#pragma once

#include <cstddef>

namespace lsql::iface::sql::parse {

struct Token {
    int code;
    const char* text;
};

}  // namespace lsql::iface::sql::parse
