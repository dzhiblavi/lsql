#pragma once

#include "sql/ast/Node.h"
#include "sql/parser/token.h"

#include <cstddef>
#include <memory>

namespace lsql::sql::parse {

struct Context {
    std::unique_ptr<ast::Node> root;
    int has_error;
};

}  // namespace lsql::sql::parse

// Lemon parser functions (sql_grammar.cpp)
void* ParseAlloc(void* (*mallocProc)(size_t));
void ParseFree(void* pParser, void (*freeProc)(void*));
void Parse(
    void* yyp, int yymajor, lsql::sql::parse::Token yyminor, lsql::sql::parse::Context* pCtx);
