#pragma once

#include "front/sql/parser/Context.h"
#include "front/sql/parser/Token.h"

#include <cstddef>

// Lemon parser functions (generated)
void* SqlParserAlloc(void* (*mallocProc)(size_t));

void SqlParserFree(void* pParser, void (*freeProc)(void*));

void SqlParser(
    void* yyp,
    int yymajor,
    lsql::front::sql::parse::Token yyminor,
    lsql::front::sql::parse::Context* pCtx);
