#pragma once

#include "sql/parser/Context.h"
#include "sql/parser/lexer/lex.yy.h"  // IWYU pragma: keep

namespace lsql::sql::parse {

void setParserContext(void* parser, Context* ctx);

}  // namespace lsql::sql::parse
