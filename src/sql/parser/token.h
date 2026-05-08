#pragma once

namespace lsql::sql::parse {

// Structure passed between lexer and parser
struct Token {
    int code;
    const char* text;
    int length;
};

}  // namespace lsql::sql::parse
