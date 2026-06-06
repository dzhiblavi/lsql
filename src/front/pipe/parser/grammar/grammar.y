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

    #include "core/ValueType.h"

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <vector>

    namespace ast = lsql::front::pipe::ast;
    namespace common = lsql::front::common;

    using lsql::Box;
    using lsql::ValueType;
    using lsql::front::common::ast::Literal;

    static std::string unquote(std::string s) {
        if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
            return s.substr(1, s.size() - 2);
        }
        return s;
    }
}

%token TOKEN_FILE.
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

%type alias           {std::string*}
%type statement       {ast::Statement*}
%type statements      {std::vector<ast::Statement>*}
%type pipeline        {ast::Pipeline*}
%type pipelines       {std::vector<ast::Pipeline>*}
%type source          {ast::Source*}
%type stage           {ast::Stage*}
%type stage_list      {std::vector<ast::Stage>*}
%type expression      {ast::Expr*}
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
    S = new ast::Statement(ast::QueryStatement{
        .pipeline = Box<ast::Pipeline>(P),
    });
}

statement(S) ::= TOKEN_IDENTIFIER(Name) TOKEN_EQ pipeline(P). {
    S = new ast::Statement(ast::NamedPipelineStatement{
        .name = Name.text,
        .pipeline = Box<ast::Pipeline>(P),
    });
}

pipeline(P) ::= source(S) stage_list(L). {
    P = new ast::Pipeline{
        .source = Box<ast::Source>(S),
        .stages = std::move(*L),
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

source(S) ::= TOKEN_DOLLAR TOKEN_IDENTIFIER(Name). {
    S = new ast::Source(ast::NamedPipelineReferenceSource{
        .name = Name.text,
    });
}

source(S) ::= TOKEN_FILE TOKEN_PATH(P). {
    S = new ast::Source(ast::FileSource{
        .path = P.text,
    });
}

source(S) ::= TOKEN_FILE TOKEN_PATH(P) TOKEN_AT TOKEN_TIMESTAMP(Ts) TOKEN_PLUS TOKEN_INTEGER(I). {
    S = new ast::Source(ast::FileIntervalSource{
        .path = P.text,
        .ts_from = Ts.text,
        .interval_s = std::stoi(I.text),
    });
}

source(S) ::= TOKEN_VALUES TOKEN_LPAREN value_list(V) TOKEN_RPAREN. {
    S = new ast::Source(ast::AdhocSource{
        .literals = std::move(*V),
    });
    delete V;
}

source(S) ::= TOKEN_UNION TOKEN_LPAREN pipeline(L) TOKEN_RPAREN TOKEN_LPAREN pipeline(R) TOKEN_RPAREN. {
    S = new ast::Source(ast::UnionAllSource{
        .left = Box<ast::Pipeline>(L),
        .right = Box<ast::Pipeline>(R),
    });
}

source(S) ::= TOKEN_MERGE TOKEN_BY expression_list(E) desc_opt(D)
              TOKEN_LPAREN pipeline(L) TOKEN_RPAREN
              TOKEN_LPAREN pipeline(R) TOKEN_RPAREN. {
    S = new ast::Source(ast::UnionAllSortedBySource{
        .left = Box<ast::Pipeline>(L),
        .right = Box<ast::Pipeline>(R),
        .order_list = std::move(*E),
        .desc = D,
    });
    delete E;
}

stage(S) ::= TOKEN_WHERE expression(E). {
    S = new ast::Stage(ast::FilterStage{
        .condition = Box<ast::Expr>(E),
    });
}

stage(S) ::= TOKEN_WHERE expression(E) TOKEN_IN TOKEN_LPAREN pipeline(P) TOKEN_RPAREN. {
    S = new ast::Stage(ast::WhereInStage{
        .expr = Box<ast::Expr>(E),
        .match = Box<ast::Pipeline>(P),
    });
}

stage(S) ::= TOKEN_TAKE TOKEN_INTEGER(N). {
    S = new ast::Stage(ast::TakeStage{
        .count = std::stoi(N.text),
    });
}

stage(S) ::= TOKEN_SORT TOKEN_BY expression_list(L) desc_opt(D). {
    S = new ast::Stage(ast::SortStage{
        .order_list = std::move(*L),
        .desc = D,
    });
    delete L;
}

stage(S) ::= TOKEN_SELECT projector_list(P) TOKEN_GROUP TOKEN_BY projector_list(G). {
    S = new ast::Stage(ast::GroupStage{
        .projectors = std::move(*P),
        .group_list = std::move(*G),
    });
    delete P;
    delete G;
}

stage(S) ::= TOKEN_SELECT projector_list(L). {
    S = new ast::Stage(ast::SelectStage{
        .projectors = std::move(*L),
    });
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

projector(P) ::= TOKEN_STAR. {
    P = new ast::Projector(ast::StarProjector{});
}

projector(P) ::= TOKEN_IDENTIFIER(Id). {
    P = new ast::Projector(ast::IdentifierProjector{
        .identifier = Id.text,
    });
}

projector(P) ::= expression(E) TOKEN_AS alias(A). {
    P = new ast::Projector(ast::ExprProjector{
        .alias = std::move(*A),
        .expr = Box<ast::Expr>(E),
    });
    delete A;
}

alias(A) ::= TOKEN_IDENTIFIER(Id). { A = new std::string(Id.text); }
alias(A) ::= TOKEN_COUNT. { A = new std::string("count"); }
alias(A) ::= TOKEN_GROUP. { A = new std::string("group"); }

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
    E = new ast::Expr(ast::LiteralExpr{
        .literal = std::move(*V),
    });
    delete V;
}

expression(E) ::= TOKEN_IDENTIFIER(Id). {
    E = new ast::Expr(ast::IdentifierExpr{
        .identifier = Id.text,
    });
}

expression(E) ::= TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    E = X;
}

expression(E) ::= TOKEN_EXCLAMATION expression(X). {
    E = new ast::Expr(ast::UnaryExpr{
        .type = common::ast::UnaryExprType::Not,
        .expr = Box<ast::Expr>(X),
    });
}

expression(E) ::= TOKEN_NOT expression(X). {
    E = new ast::Expr(ast::UnaryExpr{
        .type = common::ast::UnaryExprType::Not,
        .expr = Box<ast::Expr>(X),
    });
}

expression(E) ::= expression(L) TOKEN_EQ expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::Equal,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_NEQ expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::NotEqual,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_AND expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::And,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_OR expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::Or,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_DIVIDE expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::Divide,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_PLUS expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::Plus,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(L) TOKEN_MINUS expression(R). {
    E = new ast::Expr(ast::BinaryExpr{
        .type = common::ast::BinaryExprType::Minus,
        .left = Box<ast::Expr>(L),
        .right = Box<ast::Expr>(R),
    });
}

expression(E) ::= expression(X) TOKEN_LIKE TOKEN_STR(R). {
    auto str = std::string(R.text);
    E = new ast::Expr(ast::LikeExpr{
        .expr = Box<ast::Expr>(X),
        .regex = str.substr(1, str.size() - 2),
    });
}

expression(E) ::= expression(X) TOKEN_IN TOKEN_LPAREN pipeline(P) TOKEN_RPAREN. {
    E = new ast::Expr(ast::InExpr{
        .expr = Box<ast::Expr>(X),
        .match = Box<ast::Pipeline>(P),
    });
}

expression(E) ::= TOKEN_STRING TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_string", .args = std::move(args)});
}

expression(E) ::= TOKEN_INT TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_int", .args = std::move(args)});
}

expression(E) ::= TOKEN_FLOAT TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_float", .args = std::move(args)});
}

expression(E) ::= TOKEN_BOOL TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_bool", .args = std::move(args)});
}

expression(E) ::= TOKEN_COUNT TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_count_nonnull", .args = std::move(args)});
}

expression(E) ::= TOKEN_COUNT TOKEN_LPAREN TOKEN_STAR TOKEN_RPAREN. {
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_count_all", .args = {}});
}

expression(E) ::= TOKEN_MIN TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_min", .args = std::move(args)});
}

expression(E) ::= TOKEN_MAX TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_max", .args = std::move(args)});
}

expression(E) ::= TOKEN_SUM TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;
    E = new ast::Expr(ast::FnCallExpr{.func = "builtin_sum", .args = std::move(args)});
}

expression(E) ::= TOKEN_PERCENTILE TOKEN_LPAREN expression(X) TOKEN_COMMA expression_list(L) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;

    for (auto&& p : *L) {
        args.push_back(std::move(p));
    }
    delete L;

    E = new ast::Expr(ast::FnCallExpr{
        .func = "builtin_percentile",
        .args = std::move(args),
    });
}

expression(E) ::= TOKEN_COALESCE TOKEN_LPAREN expression_list(L) TOKEN_RPAREN. {
    E = new ast::Expr(ast::FnCallExpr{
        .func = "builtin_coalesce",
        .args = std::move(*L),
    });
    delete L;
}

expression(E) ::= TOKEN_RSUBSTR TOKEN_LPAREN expression(X) TOKEN_COMMA TOKEN_STR(P) TOKEN_RPAREN. {
    std::vector<ast::Expr> args;
    args.push_back(std::move(*X));
    delete X;

    args.emplace_back(ast::LiteralExpr{
        .literal = Literal{
            .type = ValueType::String,
            .value_str = P.text,
        },
    });

    E = new ast::Expr(ast::FnCallExpr{
        .func = "builtin_rsubstr",
        .args = std::move(args),
    });
}

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
    };
}

value(V) ::= TOKEN_INTEGER(I). {
    V = new Literal{
        .type = ValueType::Integer,
        .value_str = I.text,
    };
}

value(V) ::= TOKEN_FLOATING(F). {
    V = new Literal{
        .type = ValueType::Floating,
        .value_str = F.text,
    };
}

value(V) ::= TOKEN_TRUE(T). {
    V = new Literal{
        .type = ValueType::Boolean,
        .value_str = T.text,
    };
}

value(V) ::= TOKEN_FALSE(T). {
    V = new Literal{
        .type = ValueType::Boolean,
        .value_str = T.text,
    };
}

value(V) ::= TOKEN_NULL. {
    V = new Literal{
        .type = ValueType::Null,
        .value_str = "",
    };
}

%syntax_error {
    const char* token_text = TOKEN.text;
    int token_code = TOKEN.code;
    fprintf(stderr, "Pipe syntax error near token: '%s' (code %d)\n", token_text, token_code);
    pCtx->has_error = true;
}
