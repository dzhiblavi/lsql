%token_type     {lsql::front::pipe::parse::Token}
%default_type   {void*}
%extra_argument {lsql::front::pipe::parse::Context *pCtx}
%name PipeParser

%include {
    #include "front/pipe/ast/Expressions.h"
    #include "front/pipe/ast/Pipeline.h"
    #include "front/pipe/ast/Sources.h"
    #include "front/pipe/ast/Stages.h"

    #include "front/pipe/parser/Token.h"
    #include "front/pipe/parser/grammar/parse.h"

    #include "core/value/ValueType.h"

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <vector>

    namespace ast = lsql::front::pipe::ast;
    namespace common = lsql::front::common;

    using lsql::Box;
    using lsql::ValueType;
    using lsql::front::common::ast::Literal;
    using lsql::front::pipe::parse::Token;
    using lsql::front::spanOf;

    static std::string unquote(std::string s) {
        if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
            return s.substr(1, s.size() - 2);
        }
        return s;
    }

    std::string functionName(std::string text) {
        return text;
    }
}

%token TOKEN_FILE.
%token TOKEN_STREAM.
%token TOKEN_VALUES.
%token TOKEN_UNION.
%token TOKEN_MERGE.
%token TOKEN_WHERE.
%token TOKEN_TAKE.
%token TOKEN_SORT.
%token TOKEN_GROUP.
%token TOKEN_SELECT.
%token TOKEN_BY.
%token TOKEN_AS.
%token TOKEN_IN.
%token TOKEN_LIKE.
%token TOKEN_AND.
%token TOKEN_OR.
%token TOKEN_NOT.
%token TOKEN_ASC.
%token TOKEN_DESC.
%token TOKEN_TRUE.
%token TOKEN_FALSE.
%token TOKEN_NULL.
%token TOKEN_STRING.
%token TOKEN_INT.
%token TOKEN_FLOAT.
%token TOKEN_BOOL.
%token TOKEN_COUNT.
%token TOKEN_MIN.
%token TOKEN_MAX.
%token TOKEN_SUM.
%token TOKEN_PERCENTILE.
%token TOKEN_COALESCE.
%token TOKEN_RSUBSTR.
%token TOKEN_PARSE_TIMESTAMP.
%token TOKEN_IDENTIFIER.
%token TOKEN_PATH.
%token TOKEN_INTEGER.
%token TOKEN_FLOATING.
%token TOKEN_STR.
%token TOKEN_TIMESTAMP.
%token TOKEN_SEMICOLON.
%token TOKEN_COMMA.
%token TOKEN_PIPE.
%token TOKEN_LPAREN.
%token TOKEN_RPAREN.
%token TOKEN_EQ.
%token TOKEN_NEQ.
%token TOKEN_EXCLAMATION.
%token TOKEN_PLUS.
%token TOKEN_MINUS.
%token TOKEN_DIVIDE.
%token TOKEN_AT.
%token TOKEN_DOLLAR.
%token TOKEN_STAR.
%token TOKEN_EOF.

%left TOKEN_OR.
%left TOKEN_AND.
%right TOKEN_NOT.
%left TOKEN_EQ TOKEN_NEQ TOKEN_LIKE TOKEN_IN.
%left TOKEN_PLUS TOKEN_MINUS.
%left TOKEN_DIVIDE.
%right TOKEN_EXCLAMATION.

%type alias           {Token*}
%type statement       {ast::Statement*}
%type statements      {std::vector<ast::Statement>*}
%type pipeline        {ast::Pipeline*}
%type pipelines       {std::vector<ast::Pipeline>*}
%type source          {ast::Source*}
%type stage           {ast::Stage*}
%type stage_list      {std::vector<ast::Stage>*}
%type expression      {ast::Expr*}
%type function_name   {Token*}
%type expression_list {std::vector<ast::Expr>*}
%type projector       {ast::Projector*}
%type projector_list  {std::vector<ast::Projector>*}
%type value           {Literal*}
%type value_list      {std::vector<Literal>*}
%type desc_opt        {bool}

%start_symbol input

input ::= statements(Ss) TOKEN_EOF. {
    pCtx->program.statements = std::move(*Ss);
    delete Ss;
}

statements(Ps) ::= statement(P). {
    Ps = new std::vector<ast::Statement>();
    Ps->push_back(std::move(*P));
    delete P;
}

statements(Os) ::= statements(Ps) statement(P). {
    Os = Ps;
    Os->push_back(std::move(*P));
    delete P;
}

statement(S) ::= pipeline(P). {
    S = new ast::Statement{
        .node = ast::QueryStatement{ .pipeline = Box<ast::Pipeline>(P) },
        .span = P->span,
    };
}

statement(S) ::= TOKEN_IDENTIFIER(Name) TOKEN_EQ pipeline(P). {
    S = new ast::Statement{
        .node = ast::NamedPipelineStatement{
            .name = Name.text,
            .pipeline = Box<ast::Pipeline>(P),
        },
        .span  = merge(Name.span, P->span),
    };
}

pipeline(P) ::= source(S) stage_list(L). {
    auto span = merge(S->span, spanOf(*L));

    P = new ast::Pipeline{
        .source = Box<ast::Source>(S),
        .stages = std::move(*L),
        .span = span,
    };
    delete L;
}

stage_list(L) ::= . {
    L = new std::vector<ast::Stage>();
}

stage_list(L) ::= stage_list(LS) TOKEN_PIPE stage(S). {
    L = LS;
    L->push_back(std::move(*S));
    delete S;
}

source(S) ::= TOKEN_DOLLAR(D) TOKEN_IDENTIFIER(Name). {
    S = new ast::Source{
        .node = ast::NamedPipelineReferenceSource{ .name = Name.text },
        .span = merge(D.span, Name.span),
    };
}

source(S) ::= TOKEN_FILE(F) TOKEN_PATH(P). {
    S = new ast::Source{
        .node = ast::FileSource{ .path = P.text },
        .span = merge(F.span, P.span),
    };
}

source(S) ::= TOKEN_FILE(F) TOKEN_PATH(P) TOKEN_AT TOKEN_TIMESTAMP(Ts) TOKEN_PLUS TOKEN_INTEGER(I). {
    S = new ast::Source{
        .node = ast::FileIntervalSource{
            .path = P.text,
            .ts_from = Ts.text,
            .interval_s = std::stoi(I.text),
        },
        .span = merge(F.span, I.span),
    };
}

source(S) ::= TOKEN_STREAM(Stream) TOKEN_STR(Command). {
    S = new ast::Source{
        .node = ast::StreamSource{ .command = unquote(Command.text) },
        .span = merge(Stream.span, Command.span),
    };
}

source(S) ::= TOKEN_VALUES(Vs) TOKEN_LPAREN value_list(V) TOKEN_RPAREN(RP). {
    auto span = merge(Vs.span, RP.span);
    S = new ast::Source{
        .node = ast::AdhocSource{ .literals = std::move(*V) },
        .span = span,
    };
    delete V;
}

source(S) ::= TOKEN_UNION(U) TOKEN_LPAREN pipeline(L) TOKEN_RPAREN TOKEN_LPAREN pipeline(R) TOKEN_RPAREN(RP). {
    S = new ast::Source{
        .node = ast::UnionAllSource{
            .left = Box<ast::Pipeline>(L),
            .right = Box<ast::Pipeline>(R),
        },
        .span = merge(U.span, RP.span),
    };
}

source(S) ::= TOKEN_MERGE(M) TOKEN_BY expression_list(E) desc_opt(D)
              TOKEN_LPAREN pipeline(L) TOKEN_RPAREN
              TOKEN_LPAREN pipeline(R) TOKEN_RPAREN(RP). {
    S = new ast::Source{
        .node = ast::UnionAllSortedBySource{
            .left = Box<ast::Pipeline>(L),
            .right = Box<ast::Pipeline>(R),
            .order_list = std::move(*E),
            .desc = D,
        },
        .span = merge(M.span, RP.span),
    };
    delete E;
}

stage(S) ::= TOKEN_WHERE(W) expression(E). {
    S = new ast::Stage{
        .node = ast::FilterStage{ .condition = Box<ast::Expr>(E) },
        .span = merge(W.span, E->span),
    };
}

stage(S) ::= TOKEN_WHERE(W) expression(E) TOKEN_IN TOKEN_LPAREN pipeline(P) TOKEN_RPAREN(RP). {
    S = new ast::Stage{
        .node = ast::WhereInStage{
            .expr = Box<ast::Expr>(E),
            .match = Box<ast::Pipeline>(P),
        },
        .span = merge(W.span, RP.span),
    };
}

stage(S) ::= TOKEN_TAKE(T) TOKEN_INTEGER(N). {
    S = new ast::Stage{
        .node = ast::TakeStage{ .count = std::stoi(N.text) },
        .span = merge(T.span, N.span),
    };
}

stage(S) ::= TOKEN_SORT(ST) TOKEN_BY expression_list(L) desc_opt(D). {
    auto span = merge(ST.span, spanOf(*L));
    S = new ast::Stage{
        .node = ast::SortStage{
            .order_list = std::move(*L),
            .desc = D,
        },
        .span = span,
    };
    delete L;
}

stage(S) ::= TOKEN_SELECT(SE) projector_list(P) TOKEN_GROUP TOKEN_BY projector_list(G). {
    auto span = merge(SE.span, spanOf(*G));
    S = new ast::Stage{
        .node = ast::GroupStage{
            .projectors = std::move(*P),
            .group_list = std::move(*G),
        },
        .span = span,
    };
    delete P;
    delete G;
}

stage(S) ::= TOKEN_SELECT(SE) projector_list(L). {
    auto span = merge(SE.span, spanOf(*L));
    S = new ast::Stage{
        .node = ast::SelectStage{ .projectors = std::move(*L) },
        .span = span,
    };
    delete L;
}

desc_opt(D) ::= . { D = false; }
desc_opt(D) ::= TOKEN_ASC. { D = false; }
desc_opt(D) ::= TOKEN_DESC. { D = true; }

projector_list(L) ::= projector(P). {
    L = new std::vector<ast::Projector>();
    L->push_back(std::move(*P));
    delete P;
}

projector_list(L) ::= projector_list(PL) TOKEN_COMMA projector(P). {
    L = PL;
    L->push_back(std::move(*P));
    delete P;
}

projector(P) ::= TOKEN_STAR(S). {
    P = new ast::Projector{
        .node = ast::StarProjector{},
        .span = S.span,
    };
}

projector(P) ::= TOKEN_IDENTIFIER(Id). {
    P = new ast::Projector{
        .node = ast::IdentifierProjector{ .identifier = Id.text },
        .span = Id.span,
    };
}

projector(P) ::= expression(E) TOKEN_AS alias(A). {
    auto span = merge(E->span, A->span);
    P = new ast::Projector{
        .node = ast::ExprProjector{
            .alias = std::move(A->text),
            .expr = Box<ast::Expr>(E),
        },
        .span = span,
    };
    delete A;
}

alias(A) ::= TOKEN_IDENTIFIER(Id). { A = new Token(Id); }
alias(A) ::= TOKEN_COUNT(T). { A = new Token(T); }
alias(A) ::= TOKEN_GROUP(T). { A = new Token(T); }

expression_list(L) ::= expression(E). {
    L = new std::vector<ast::Expr>();
    L->push_back(std::move(*E));
    delete E;
}

expression_list(L) ::= expression_list(EL) TOKEN_COMMA expression(E). {
    L = EL;
    L->push_back(std::move(*E));
    delete E;
}

expression(E) ::= value(V). {
    auto span = V->span;
    E = new ast::Expr{
        .node = ast::LiteralExpr{ .literal = std::move(*V) },
        .span = span,
    };
    delete V;
}

expression(E) ::= TOKEN_IDENTIFIER(Id). {
    E = new ast::Expr{
        .node = ast::IdentifierExpr{ .identifier = Id.text },
        .span = Id.span,
    };
}

expression(E) ::= TOKEN_LPAREN(LP) expression(X) TOKEN_RPAREN(RP). {
    E = X;
    E->span = merge(LP.span, RP.span);
}

expression(E) ::= TOKEN_EXCLAMATION(Ex) expression(X). {
    auto span = merge(Ex.span, X->span);
    E = new ast::Expr{
        .node = ast::UnaryExpr{
            .type = common::ast::UnaryExprType::Not,
            .expr = Box<ast::Expr>(X),
        },
        .span = span,
    };
}

expression(E) ::= TOKEN_NOT(N) expression(X). {
    auto span = merge(N.span, X->span);
    E = new ast::Expr{
        .node = ast::UnaryExpr{
            .type = common::ast::UnaryExprType::Not,
            .expr = Box<ast::Expr>(X),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_EQ expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Equal,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_NEQ expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::NotEqual,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_AND expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::And,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_OR expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Or,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_DIVIDE expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Divide,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_PLUS expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Plus,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(L) TOKEN_MINUS expression(R). {
    auto span = merge(L->span, R->span);
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Minus,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = span,
    };
}

expression(E) ::= expression(X) TOKEN_LIKE TOKEN_STR(R). {
    auto span = merge(X->span, R.span);
    auto str = std::string(R.text);
    E = new ast::Expr{
        .node = ast::LikeExpr{
            .expr = Box<ast::Expr>(X),
            .regex = str.substr(1, str.size() - 2),
        },
        .span = span,
    };
}

expression(E) ::= expression(X) TOKEN_IN TOKEN_LPAREN pipeline(P) TOKEN_RPAREN(RP). {
    auto span = merge(X->span, RP.span);
    E = new ast::Expr{
        .node = ast::InExpr{
            .expr = Box<ast::Expr>(X),
            .match = Box<ast::Pipeline>(P),
        },
        .span = span,
    };
}

expression(E) ::= TOKEN_COUNT(I) TOKEN_LPAREN TOKEN_STAR TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "count_all",
            .args = {},
        },
        .span = merge(I.span, RP.span),
    };
}

expression(E) ::= TOKEN_COUNT(I) TOKEN_LPAREN expression_list(L) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "count_nonnull",
            .args = std::move(*L),
        },
        .span = merge(I.span, RP.span),
    };

    delete L;
}

expression(E) ::= function_name(Fn) TOKEN_LPAREN expression_list(L) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = functionName(Fn->text),
            .args = std::move(*L),
        },
        .span = merge(Fn->span, RP.span),
    };

    delete Fn;
    delete L;
}

function_name(F) ::= TOKEN_STRING(T). { F = new Token(T); }
function_name(F) ::= TOKEN_INT(T). { F = new Token(T); }
function_name(F) ::= TOKEN_FLOAT(T). { F = new Token(T); }
function_name(F) ::= TOKEN_BOOL(T). { F = new Token(T); }
function_name(F) ::= TOKEN_MIN(T). { F = new Token(T); }
function_name(F) ::= TOKEN_MAX(T). { F = new Token(T); }
function_name(F) ::= TOKEN_SUM(T). { F = new Token(T); }
function_name(F) ::= TOKEN_PERCENTILE(T). { F = new Token(T); }
function_name(F) ::= TOKEN_COALESCE(T). { F = new Token(T); }
function_name(F) ::= TOKEN_RSUBSTR(T). { F = new Token(T); }
function_name(F) ::= TOKEN_PARSE_TIMESTAMP(T). { F = new Token(T); }

value_list(L) ::= value(V). {
    L = new std::vector<Literal>();
    L->push_back(std::move(*V));
    delete V;
}

value_list(L) ::= value_list(L1) TOKEN_COMMA value(V). {
    L = L1;
    L->push_back(std::move(*V));
    delete V;
}

value(V) ::= TOKEN_STR(S). {
    V = new Literal{
        .type = ValueType::String,
        .value_str = S.text,
        .span = S.span,
    };
}

value(V) ::= TOKEN_INTEGER(I). {
    V = new Literal{
        .type = ValueType::Integer,
        .value_str = I.text,
        .span = I.span,
    };
}

value(V) ::= TOKEN_FLOATING(F). {
    V = new Literal{
        .type = ValueType::Floating,
        .value_str = F.text,
        .span = F.span,
    };
}

value(V) ::= TOKEN_TRUE(T). {
    V = new Literal{
        .type = ValueType::Boolean,
        .value_str = T.text,
        .span = T.span,
    };
}

value(V) ::= TOKEN_FALSE(T). {
    V = new Literal{
        .type = ValueType::Boolean,
        .value_str = T.text,
        .span = T.span,
    };
}

value(V) ::= TOKEN_NULL(N). {
    V = new Literal{
        .type = ValueType::Null,
        .value_str = N.text,
        .span = N.span,
    };
}

%syntax_error {
    pCtx->has_error = true;
    pCtx->error_span = TOKEN.span;
}
