#pragma once

#include "front/pipe/parser/Context.h"
#include "front/pipe/parser/Token.h"

#include <cstddef>

// Lemon parser functions (generated)
void* PipeParserAlloc(void* (*mallocProc)(size_t));

void PipeParserFree(void* pParser, void (*freeProc)(void*));

void PipeParser(
    void* yyp,
    int yymajor,
    lsql::front::pipe::parse::Token yyminor,
    lsql::front::pipe::parse::Context* pCtx);
