%option c++
%option noyywrap
%option never-interactive
%option prefix="sql_"

%top {
#include "front/common/source/SourceSpan.h"
void sqlSetLocPtr(lsql::front::SourceSpan* ptr);
}

%{

#include "front/sql/parser/grammar/grammar.h"

static lsql::front::SourcePos current_pos;
static lsql::front::SourceSpan* yylloc;

void sqlSetLocPtr(lsql::front::SourceSpan* ptr) {
    yylloc = ptr;
}

#define YY_USER_ACTION                                     \
    yylloc->begin = current_pos;                           \
    for (int i = 0; i < yyleng; ++i) {                     \
        if (yytext[i] == '\n') {                           \
            ++current_pos.line;                            \
            current_pos.column = 1;                        \
        } else {                                           \
            ++current_pos.column;                          \
        }                                                  \
        ++current_pos.offset;                              \
    }                                                      \
    yylloc->end = current_pos;

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

";"              { return TOKEN_SEMICOLON; }
","              { return TOKEN_COMMA; }
"*"              { return TOKEN_STAR; }
"="              { return TOKEN_EQ; }
"!="             { return TOKEN_NEQ; }
"("              { return TOKEN_LPAREN; }
")"              { return TOKEN_RPAREN; }
"!"              { return TOKEN_EXCLAMATION; }
"+"              { return TOKEN_PLUS; }
"-"              { return TOKEN_MINUS; }
"/"              { return TOKEN_DIVIDE; }
"@"              { return TOKEN_AT; }
"$"              { return TOKEN_DOLLAR; }

SELECT           { return TOKEN_SELECT; }
FROM             { return TOKEN_FROM; }
AS               { return TOKEN_AS; }
WHERE            { return TOKEN_WHERE; }
LIMIT            { return TOKEN_LIMIT; }
AND              { return TOKEN_AND; }
OR               { return TOKEN_OR; }
LIKE             { return TOKEN_LIKE; }
MATERIALIZE      { return TOKEN_MATERIALIZE; }
String           { return TOKEN_STRING; }
Int              { return TOKEN_INT; }
Float            { return TOKEN_FLOAT; }
Bool             { return TOKEN_BOOL; }
COUNT            { return TOKEN_COUNT; }
true             { return TOKEN_TRUE; }
false            { return TOKEN_FALSE; }
null             { return TOKEN_NULL; }
ASC              { return TOKEN_ASC; }
DESC             { return TOKEN_DESC; }
MIN              { return TOKEN_MIN; }
MAX              { return TOKEN_MAX; }
SUM              { return TOKEN_SUM; }
IN               { return TOKEN_IN; }
NOT              { return TOKEN_NOT; }
PERCENTILE       { return TOKEN_PERCENTILE; }
COALESCE         { return TOKEN_COALESCE; }
RSUBSTR          { return TOKEN_RSUBSTR; }
"GROUP BY"       { return TOKEN_GROUP_BY; }
"ORDER BY"       { return TOKEN_ORDER_BY; }
"UNION ALL"      { return TOKEN_UNION_ALL; }
"UNION ALL SORTED BY" { return TOKEN_UNION_ALL_SORTED_BY; }
{INTEGER}        { return TOKEN_INTEGER; }
{FLOATING}       { return TOKEN_FLOATING; }
{IDENTIFIER}     { return TOKEN_IDENTIFIER; }
{PATH}           { return TOKEN_PATH; }
{QUOTED_STRING}  { return TOKEN_STR; }
{TIMESTAMP}      { return TOKEN_TIMESTAMP; }
<<EOF>>          { return TOKEN_EOF; }
.                { return -1; }

%%
