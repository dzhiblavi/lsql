#pragma once

#include "front/sql/parser/Context.h"
#include "front/sql/parser/Token.h"

#include <cstddef>

// Lemon parser functions (sql_grammar.cpp)
void* ParseAlloc(void* (*mallocProc)(size_t));

void ParseFree(void* pParser, void (*freeProc)(void*));

void Parse(
    void* yyp,
    int yymajor,
    lsql::front::sql::parse::Token yyminor,
    lsql::front::sql::parse::Context* pCtx);
