#pragma once

#include "iface/sql/parser/Context.h"
#include "iface/sql/parser/Token.h"

#include <cstddef>

// Lemon parser functions (sql_grammar.cpp)
void* ParseAlloc(void* (*mallocProc)(size_t));

void ParseFree(void* pParser, void (*freeProc)(void*));

void Parse(
    void* yyp,
    int yymajor,
    lsql::iface::sql::parse::Token yyminor,
    lsql::iface::sql::parse::Context* pCtx);
