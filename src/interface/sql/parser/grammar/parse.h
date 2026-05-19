#pragma once

#include "interface/sql/parser/Context.h"
#include "interface/sql/parser/Token.h"

#include <cstddef>

// Lemon parser functions (sql_grammar.cpp)
void* ParseAlloc(void* (*mallocProc)(size_t));
void ParseFree(void* pParser, void (*freeProc)(void*));
void Parse(
    void* yyp, int yymajor, lsql::sql::parse::Token yyminor, lsql::sql::parse::Context* pCtx);
