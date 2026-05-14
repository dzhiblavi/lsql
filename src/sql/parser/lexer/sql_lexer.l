%option noyywrap reentrant
%{

#include "sql/parser/grammar/sql_grammar.h"
#include "sql/parser/Token.h"
#include "sql/parser/grammar/parse.h"

#include <string.h>

namespace {

static void* g_parser = nullptr;
static lsql::sql::parse::Context* g_parse_context = nullptr;

} // namespace

%}

WHITESPACE       [ \t\n]+
IDENTIFIER       [a-zA-Z_][a-zA-Z0-9_]*
PATH             [/\\]?([a-zA-Z0-9_\-\.]+[/\\]+)*[a-zA-Z0-9_\-\.]+
INTEGER          [0-9]+
FLOATING         [+-]?[0-9]+\.[0-9]+
QUOTED_STRING    '[^']*'
TIMESTAMP        [0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}

%%

{WHITESPACE}     { /* ignore */ }

"--".*           { /* ignore */  }

SELECT           {
                     lsql::sql::parse::Token t = {TOKEN_SELECT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_SELECT, t, g_parse_context);
                 }

FROM             {
                     lsql::sql::parse::Token t = {TOKEN_FROM, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_FROM, t, g_parse_context);
                 }

AS               {
                     lsql::sql::parse::Token t = {TOKEN_AS, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_AS, t, g_parse_context);
                 }

WHERE            {
                     lsql::sql::parse::Token t = {TOKEN_WHERE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_WHERE, t, g_parse_context);
                 }

LIMIT            {
                     lsql::sql::parse::Token t = {TOKEN_LIMIT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_LIMIT, t, g_parse_context);
                 }

AND              {
                     lsql::sql::parse::Token t = {TOKEN_AND, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_AND, t, g_parse_context);
                 }

OR               {
                     lsql::sql::parse::Token t = {TOKEN_OR, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_OR, t, g_parse_context);
                 }

LIKE             {
                     lsql::sql::parse::Token t = {TOKEN_LIKE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_LIKE, t, g_parse_context);
                 }

MATERIALIZE      {
                     lsql::sql::parse::Token t = {TOKEN_MATERIALIZE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_MATERIALIZE, t, g_parse_context);
                 }

String           {
                     lsql::sql::parse::Token t = {TOKEN_STRING, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_STRING, t, g_parse_context);
                 }

Int              {
                     lsql::sql::parse::Token t = {TOKEN_INT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_INT, t, g_parse_context);
                 }

Float            {
                     lsql::sql::parse::Token t = {TOKEN_FLOAT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_FLOAT, t, g_parse_context);
                 }

Bool             {
                     lsql::sql::parse::Token t = {TOKEN_BOOL, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_BOOL, t, g_parse_context);
                 }

COUNT            {
                     lsql::sql::parse::Token t = {TOKEN_COUNT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_COUNT, t, g_parse_context);
                 }

true             {
                     lsql::sql::parse::Token t = {TOKEN_TRUE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_TRUE, t, g_parse_context);
                 }

false            {
                     lsql::sql::parse::Token t = {TOKEN_FALSE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_FALSE, t, g_parse_context);
                 }

null             {
                     lsql::sql::parse::Token t = {TOKEN_NULL, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_NULL, t, g_parse_context);
                 }

ASC              {
                     lsql::sql::parse::Token t = {TOKEN_ASC, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_ASC, t, g_parse_context);
                 }

DESC             {
                     lsql::sql::parse::Token t = {TOKEN_DESC, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_DESC, t, g_parse_context);
                 }

MIN              {
                     lsql::sql::parse::Token t = {TOKEN_MIN, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_MIN, t, g_parse_context);
                 }

MAX              {
                     lsql::sql::parse::Token t = {TOKEN_MAX, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_MAX, t, g_parse_context);
                 }

SUM              {
                     lsql::sql::parse::Token t = {TOKEN_SUM, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_SUM, t, g_parse_context);
                 }

IN               {
                     lsql::sql::parse::Token t = {TOKEN_IN, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_IN, t, g_parse_context);
                 }

PERCENTILE       {
                     lsql::sql::parse::Token t = {TOKEN_PERCENTILE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_PERCENTILE, t, g_parse_context);
                 }

COALESCE         {
                     lsql::sql::parse::Token t = {TOKEN_COALESCE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_COALESCE, t, g_parse_context);
                 }

RSUBSTR          {
                     lsql::sql::parse::Token t = {TOKEN_RSUBSTR, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_RSUBSTR, t, g_parse_context);
                 }

"GROUP BY"       {
                     lsql::sql::parse::Token t = {TOKEN_GROUP_BY, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_GROUP_BY, t, g_parse_context);
                 }

"ORDER BY"       {
                     lsql::sql::parse::Token t = {TOKEN_ORDER_BY, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_ORDER_BY, t, g_parse_context);
                 }

"UNION ALL"      {
                     lsql::sql::parse::Token t = {TOKEN_UNION_ALL, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_UNION_ALL, t, g_parse_context);
                 }

"UNION ALL SORTED BY" {
                     lsql::sql::parse::Token t = {TOKEN_UNION_ALL_SORTED_BY, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_UNION_ALL_SORTED_BY, t, g_parse_context);
                 }

{INTEGER}        {
                     lsql::sql::parse::Token t = {TOKEN_INTEGER, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_INTEGER, t, g_parse_context);
                 }

{FLOATING}       {
                     lsql::sql::parse::Token t = {TOKEN_FLOATING, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_FLOATING, t, g_parse_context);
                 }

{IDENTIFIER}     {
                     lsql::sql::parse::Token t = {TOKEN_IDENTIFIER, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_IDENTIFIER, t, g_parse_context);
                 }

{PATH}           {
                     lsql::sql::parse::Token t = {TOKEN_PATH, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_PATH, t, g_parse_context);
                 }

{QUOTED_STRING}  {
                     lsql::sql::parse::Token t = {TOKEN_STR, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_STR, t, g_parse_context);
                 }

{TIMESTAMP}      {
                     lsql::sql::parse::Token t = {TOKEN_TIMESTAMP, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_TIMESTAMP, t, g_parse_context);
                 }

";"              {
                     lsql::sql::parse::Token t = {TOKEN_SEMICOLON, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_SEMICOLON, t, g_parse_context);
                 }

","              {
                     lsql::sql::parse::Token t = {TOKEN_COMMA, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_COMMA, t, g_parse_context);
                 }

"*"              {
                     lsql::sql::parse::Token t = {TOKEN_STAR, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_STAR, t, g_parse_context);
                 }

"="              {
                     lsql::sql::parse::Token t = {TOKEN_EQ, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_EQ, t, g_parse_context);
                 }

"!="             {
                     lsql::sql::parse::Token t = {TOKEN_NEQ, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_NEQ, t, g_parse_context);
                 }

"("              {
                     lsql::sql::parse::Token t = {TOKEN_LPAREN, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_LPAREN, t, g_parse_context);
                 }

")"              {
                     lsql::sql::parse::Token t = {TOKEN_RPAREN, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_RPAREN, t, g_parse_context);
                 }

"!"              {
                     lsql::sql::parse::Token t = {TOKEN_EXCLAMATION, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_EXCLAMATION, t, g_parse_context);
                 }

"+"              {
                     lsql::sql::parse::Token t = {TOKEN_PLUS, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_PLUS, t, g_parse_context);
                 }

"/"              {
                     lsql::sql::parse::Token t = {TOKEN_DIVIDE, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_DIVIDE, t, g_parse_context);
                 }

"@"              {
                     lsql::sql::parse::Token t = {TOKEN_AT, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_AT, t, g_parse_context);
                 }

"$"              {
                     lsql::sql::parse::Token t = {TOKEN_DOLLAR, strdup(yytext), (int)strlen(yytext)};
                     Parse(g_parser, TOKEN_DOLLAR, t, g_parse_context);
                 }

<<EOF>>          {
                     lsql::sql::parse::Token t = {TOKEN_EOF, NULL, 0};
                     Parse(g_parser, TOKEN_EOF, t, g_parse_context);
                     yyterminate();
                 }

.                {
                     fprintf(stderr, "lexer error: unknown character: '%s' (ASCII %d)\n", yytext, yytext[0]);
                     g_parse_context->has_error = 1;
                 }

%%

namespace lsql::sql::parse {

void setParserContext(void* parser, lsql::sql::parse::Context* ctx) {
    g_parser = parser;
    g_parse_context = ctx;
}

}  // namespace lsql::sql::parse
