%option c++
%option noyywrap
%option never-interactive
%option prefix="pipe_"

%top {
#include "front/common/source/SourceSpan.h"
void pipeSetLocPtr(lsql::front::SourceSpan* ptr);
}

%{

#include "front/pipe/parser/grammar/grammar.h"

static lsql::front::SourcePos current_pos;
static lsql::front::SourceSpan* yylloc;

void pipeSetLocPtr(lsql::front::SourceSpan* ptr) {
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
IDENTIFIER       \.?[a-zA-Z_][a-zA-Z0-9_]*
PATH             [/\\]?([a-zA-Z0-9_\-\.]+[/\\]+)*[a-zA-Z0-9_\-\.]+
INTEGER          [0-9]+
FLOATING         [+-]?[0-9]+\.[0-9]+
QUOTED_STRING    '[^']*'
TIMESTAMP        [0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}

%%

{WHITESPACE}     { /* ignore */ }
"--".*           { /* ignore */ }

";"              { return TOKEN_SEMICOLON; }
","              { return TOKEN_COMMA; }
"|"              { return TOKEN_PIPE; }
"("              { return TOKEN_LPAREN; }
")"              { return TOKEN_RPAREN; }
"="              { return TOKEN_EQ; }
"!="             { return TOKEN_NEQ; }
"!"              { return TOKEN_EXCLAMATION; }
"+"              { return TOKEN_PLUS; }
"-"              { return TOKEN_MINUS; }
"/"              { return TOKEN_DIVIDE; }
"@"              { return TOKEN_AT; }
"$"              { return TOKEN_DOLLAR; }
"*"              { return TOKEN_STAR; }

file             { return TOKEN_FILE; }
values           { return TOKEN_VALUES; }
union            { return TOKEN_UNION; }
merge            { return TOKEN_MERGE; }

where            { return TOKEN_WHERE; }
filter           { return TOKEN_WHERE; }
take             { return TOKEN_TAKE; }
limit            { return TOKEN_TAKE; }
sort             { return TOKEN_SORT; }
group            { return TOKEN_GROUP; }
select           { return TOKEN_SELECT; }

by               { return TOKEN_BY; }
as               { return TOKEN_AS; }
in               { return TOKEN_IN; }
like             { return TOKEN_LIKE; }
and              { return TOKEN_AND; }
or               { return TOKEN_OR; }
not              { return TOKEN_NOT; }
asc              { return TOKEN_ASC; }
desc             { return TOKEN_DESC; }

true             { return TOKEN_TRUE; }
false            { return TOKEN_FALSE; }
null             { return TOKEN_NULL; }

string           { return TOKEN_STRING; }
int              { return TOKEN_INT; }
float            { return TOKEN_FLOAT; }
bool             { return TOKEN_BOOL; }

count            { return TOKEN_COUNT; }
min              { return TOKEN_MIN; }
max              { return TOKEN_MAX; }
sum              { return TOKEN_SUM; }
percentile       { return TOKEN_PERCENTILE; }
coalesce         { return TOKEN_COALESCE; }
rsubstr          { return TOKEN_RSUBSTR; }

{TIMESTAMP}      { return TOKEN_TIMESTAMP; }
{FLOATING}       { return TOKEN_FLOATING; }
{INTEGER}        { return TOKEN_INTEGER; }
{QUOTED_STRING}  { return TOKEN_STR; }
{IDENTIFIER}     { return TOKEN_IDENTIFIER; }
{PATH}           { return TOKEN_PATH; }

<<EOF>>          { return TOKEN_EOF; }

.                { return -1; }

%%
