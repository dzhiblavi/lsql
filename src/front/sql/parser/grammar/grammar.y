%token_type     {lsql::front::sql::parse::Token}
%default_type   {void*}
%extra_argument {lsql::front::sql::parse::Context *pCtx}
%name SqlParser

%include {
    #include "front/sql/ast/Expressions.h"
    #include "front/sql/ast/Relations.h"
    #include "front/sql/ast/Statement.h"

    #include "front/sql/parser/Token.h"
    #include "front/sql/parser/grammar/parse.h"

    #include "core/ValueType.h"

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <vector>

    namespace ast = lsql::front::sql::ast;
    namespace common = lsql::front::common;

    using lsql::Box;
    using lsql::ValueType;
    using lsql::front::common::ast::Literal;
}

%token TOKEN_SELECT.
%token TOKEN_COUNT.
%token TOKEN_FROM.
%token TOKEN_IDENTIFIER.
%token TOKEN_SEMICOLON.
%token TOKEN_EOF.
%token TOKEN_AS.
%token TOKEN_IN.
%token TOKEN_COALESCE.
%token TOKEN_COMMA.
%token TOKEN_STAR.
%token TOKEN_WHERE.
%token TOKEN_LIMIT.
%token TOKEN_INTEGER.
%token TOKEN_FLOATING.
%token TOKEN_PATH.
%token TOKEN_STR.
%token TOKEN_RSUBSTR.
%token TOKEN_LPAREN.
%token TOKEN_RPAREN.
%token TOKEN_AT.
%token TOKEN_GROUP_BY.
%token TOKEN_ORDER_BY.
%token TOKEN_TRUE.
%token TOKEN_FALSE.
%token TOKEN_DESC.
%token TOKEN_ASC.
%token TOKEN_NULL.
%token TOKEN_DOLLAR.
%token TOKEN_UNION_ALL.
%token TOKEN_UNION_ALL_SORTED_BY.
%token TOKEN_MIN.
%token TOKEN_MAX.
%token TOKEN_SUM.
%token TOKEN_PERCENTILE.
%token TOKEN_PLUS.
%token TOKEN_MINUS.
%token TOKEN_LIKE.
%token TOKEN_MATERIALIZE.
%token TOKEN_STRING.
%token TOKEN_INT.
%token TOKEN_FLOAT.
%token TOKEN_BOOL.
%token TOKEN_EXCLAMATION.
%token TOKEN_NOT.
%token TOKEN_TIMESTAMP.

// Precedence (from LOWEST to HIGHEST)
%left TOKEN_UNION_ALL.
%left TOKEN_UNION_ALL_SORTED_BY.
%left TOKEN_OR.
%left TOKEN_AND.
%left TOKEN_PLUS.
%left TOKEN_MINUS.
%left TOKEN_DIVIDE.
%left TOKEN_EQ TOKEN_NEQ.
%right TOKEN_EXCLAMATION.

%type value              {Literal*}
%type expression         {ast::Expr*}
%type condition          {ast::Expr*}
%type group_expression   {ast::Expr*}
%type select_item        {ast::Projector*}
%type group_select_item  {ast::Projector*}

%type expression_list    {std::vector<ast::Expr>*}
%type value_list         {std::vector<Literal>*}
%type statement_list     {std::vector<ast::Statement>*}
%type select_list        {std::vector<ast::Projector>*}
%type group_by_list      {std::vector<ast::Projector>*}

%type where_opt          {std::optional<ast::Where>*}
%type group_by_opt       {std::optional<ast::GroupBy>*}
%type order_by_opt       {std::optional<ast::OrderBy>*}
%type limit_opt          {std::optional<ast::Limit>*}

%type statement          {ast::Statement*}
%type relation           {ast::Relation*}
%type immediate_relation {ast::Relation*}
%type adhoc_relation     {ast::Relation*}
%type file_source        {ast::Relation*}
%type select_source      {ast::Relation*}
%type select_statement   {ast::Relation*}

%start_symbol input

input ::= statement_list(L) TOKEN_EOF. {
    pCtx->program = std::move(*L);
    delete L;
}

statement_list(S) ::= statement(X). {
    S = new std::vector<ast::Statement>();
    S->push_back(std::move(*X));
    delete X;
}

statement_list(S) ::= statement_list(SL) statement(S1). {
    S = SL;
    S->push_back(std::move(*S1));
    delete S1;
}

statement(S) ::= relation(R). {
    S = new ast::Statement{
        .node = ast::QueryStatement{
            .relation = Box<ast::Relation>(R),
        },
        .span = R->span,
    };
}

statement(S) ::= TOKEN_IDENTIFIER(Name) TOKEN_EQ select_source(R). {
    S = new ast::Statement{
        .node = ast::NamedRelationStatement{
            .name = Name.text,
            .relation = Box<ast::Relation>(R),
        },
        .span = merge(Name.span, R->span),
    };
}

// immediate = does not require parentheses in SELECT FROM <here>
immediate_relation(R) ::= adhoc_relation(A). { R = A; }
immediate_relation(R) ::= file_source(F). { R = F; }

immediate_relation(R) ::= TOKEN_DOLLAR TOKEN_IDENTIFIER(Name). {
    R = new ast::Relation{
        .node = ast::NamedRelationReferenceRelation{
            .name = Name.text,
        },
        .span = Name.span,
    };
}

immediate_relation(R) ::= TOKEN_MATERIALIZE(m) TOKEN_LPAREN relation(S) TOKEN_RPAREN(r). {
    R = new ast::Relation{
        .node = ast::MaterializeRelation{
            .relation = Box<ast::Relation>(S),
        },
        .span = merge(m.span, r.span),
    };
}

// all kinds of relations that can be used on top level
relation(R) ::= immediate_relation(A). { R = A; }
relation(R) ::= select_statement(A). { R = A; }

relation(A) ::= relation(L) TOKEN_UNION_ALL relation(R). {
    A = new ast::Relation{
        .node = ast::UnionAllRelation{
            .left = Box<ast::Relation>(L),
            .right = Box<ast::Relation>(R),
        },
        .span = merge(L->span, R->span),
    };
}

relation(A) ::= relation(L) TOKEN_UNION_ALL_SORTED_BY expression_list(S) relation(R). {
    A = new ast::Relation{
        .node = ast::UnionAllSortedByRelation{
            .left = Box<ast::Relation>(L),
            .right = Box<ast::Relation>(R),
            .order_by = {
                .order_list = std::move(*S),
                .desc = false,
            },
        },
        .span = merge(L->span, R->span),
    };

    delete S;
}

relation(A) ::= relation(L) TOKEN_UNION_ALL_SORTED_BY expression_list(S) TOKEN_DESC relation(R). {
    A = new ast::Relation{
        .node = ast::UnionAllSortedByRelation{
            .left = Box<ast::Relation>(L),
            .right = Box<ast::Relation>(R),
            .order_by = {
                .order_list = std::move(*S),
                .desc = true,
            },
        },
        .span = merge(L->span, R->span),
    };

    delete S;
}

adhoc_relation(A) ::= TOKEN_LPAREN(LP) value_list(L) TOKEN_RPAREN(RP). {
    A = new ast::Relation{
        .node = ast::AdhocRelation{
            .literals = std::move(*L),
        },
        .span = merge(LP.span, RP.span),
    };

    delete L;
}

select_statement(A) ::= TOKEN_SELECT(S) select_list(L)
                        TOKEN_FROM select_source(F)
                        where_opt(Wh)
                        group_by_opt(Gr)
                        order_by_opt(Ord)
                        limit_opt(Lim).
{
    auto end_span = Lim->has_value() ? (*Lim)->span
        : Ord->has_value() ? (*Ord)->span
        : Gr->has_value() ? (*Gr)->span
        : Wh->has_value() ? (*Wh)->span
        : F->span;

    A = new ast::Relation{
        .node = ast::SelectRelation{
            .projectors = std::move(*L),
            .source = Box<ast::Relation>(F),
            .limit = std::move(*Lim),
            .where = std::move(*Wh),
            .order_by = std::move(*Ord),
            .group_by = std::move(*Gr),
        },
        .span = merge(S.span, end_span),
    };

    delete L;
    delete Lim;
    delete Wh;
    delete Ord;
    delete Gr;
}

// all relations in parentheses
select_source(S) ::= TOKEN_LPAREN(LP) relation(R) TOKEN_RPAREN(RP). {
    S = R;
    S->span = merge(LP.span, RP.span);
}

// immediate relations as is
select_source(S) ::= immediate_relation(R). { S = R; }

file_source(F) ::= TOKEN_PATH(File). {
    F = new ast::Relation{
        .node = ast::FileRelation{
            .path = File.text,
        },
        .span = File.span,
    };
}

file_source(F) ::= TOKEN_PATH(File) TOKEN_AT TOKEN_TIMESTAMP(Ts) TOKEN_PLUS TOKEN_INTEGER(Interval). {
    F = new ast::Relation{
        .node = ast::FileIntervalRelation{
            .path = File.text,
            .ts_from = Ts.text,
            .interval_s = std::stoi(Interval.text),
        },
        .span = merge(File.span, Interval.span),
    };
}

where_opt(A) ::= . { A = new std::optional<ast::Where>(); }

where_opt(A) ::= TOKEN_WHERE(W) condition(C). {
    A = new std::optional<ast::Where>(ast::Where{
        .condition = Box<ast::Expr>(C),
        .span = merge(W.span, C->span),
    });
}

group_by_opt(G) ::= . { G = new std::optional<ast::GroupBy>(); }

group_by_opt(G) ::= TOKEN_GROUP_BY(GB) group_by_list(L). {
    auto end_span = GB.span;
    if (!L->empty()) {
        end_span = L->back().span;
    }

    G = new std::optional<ast::GroupBy>(ast::GroupBy{
        .group_list = std::move(*L),
        .span = merge(GB.span, end_span),
    });

    delete L;
}

order_by_opt(G) ::= . { G = new std::optional<ast::OrderBy>(); }

order_by_opt(G) ::= TOKEN_ORDER_BY(OB) expression_list(L). {
    auto end_span = OB.span;
    if (!L->empty()) {
        end_span = L->back().span;
    }

    G = new std::optional<ast::OrderBy>(ast::OrderBy{
        .order_list = std::move(*L),
        .desc = false,
        .span = merge(OB.span, end_span),
    });

    delete L;
}

order_by_opt(G) ::= TOKEN_ORDER_BY(OB) expression_list(L) TOKEN_ASC(A). {
    G = new std::optional<ast::OrderBy>(ast::OrderBy{
        .order_list = std::move(*L),
        .desc = false,
        .span = merge(OB.span, A.span),
    });

    delete L;
}

order_by_opt(G) ::= TOKEN_ORDER_BY(OB) expression_list(L) TOKEN_DESC(D). {
    G = new std::optional<ast::OrderBy>(ast::OrderBy{
        .order_list = std::move(*L),
        .desc = true,
        .span = merge(OB.span, D.span),
    });

    delete L;
}

condition(C) ::= expression(E). { C = E; }

limit_opt(A) ::= . { A = new std::optional<ast::Limit>(); }

limit_opt(A) ::= TOKEN_LIMIT(L) TOKEN_INTEGER(N). {
    A = new std::optional<ast::Limit>(ast::Limit{
        .limit = std::stoi(N.text),
        .span = merge(L.span, N.span),
    });
}

select_list(A) ::= select_item(B). {
    A = new std::vector<ast::Projector>();
    A->push_back(std::move(*B));
    delete B;
}

select_list(A) ::= select_list(B) TOKEN_COMMA select_item(C). {
    A = B;
    A->push_back(std::move(*C));
    delete C;
}

select_item(A) ::= TOKEN_STAR(S). {
    A = new ast::Projector{
        .node = ast::StarProjector(),
        .span = S.span,
    };
}

select_item(A) ::= TOKEN_IDENTIFIER(Id). {
    A = new ast::Projector{
        .node = ast::IdentifierProjector{ .identifier = Id.text },
        .span = Id.span,
    };
}

select_item(A) ::= expression(E) TOKEN_AS TOKEN_IDENTIFIER(Id). {
    A = new ast::Projector{
        .node = ast::ExprProjector{
            .alias = Id.text,
            .expr = Box<ast::Expr>(E),
        },
        .span = merge(E->span, Id.span),
    };
}

group_by_list(A) ::= select_item(B). {
    A = new std::vector<ast::Projector>();
    A->push_back(std::move(*B));
    delete B;
}

group_by_list(A) ::= group_by_list(B) TOKEN_COMMA select_item(C). {
    A = B;
    A->push_back(std::move(*C));
    delete C;
}

expression_list(A) ::= expression(B). {
    A = new std::vector<ast::Expr>();
    A->push_back(std::move(*B));
    delete B;
}

expression_list(A) ::= expression_list(B) TOKEN_COMMA expression(C). {
    A = B;
    A->push_back(std::move(*C));
    delete C;
}

expression(E) ::= value(V). {
    E = new ast::Expr{
        .node = ast::LiteralExpr{ .literal = std::move(*V) },
        .span = V->span,
    };

    delete V;
}

expression(E) ::= TOKEN_LPAREN(LP) expression(X) TOKEN_RPAREN(RP). {
    E = X;
    E->span = merge(LP.span, RP.span);
}

expression(E) ::= TOKEN_IDENTIFIER(Id). {
    E = new ast::Expr{
        .node = ast::IdentifierExpr{ .identifier = Id.text },
        .span = Id.span,
    };
}

expression(E) ::= TOKEN_STRING(Str) TOKEN_LPAREN expression(S) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::CastExpr{
            .cast_to = ValueType::String,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(Str.span, RP.span),
    };
}

expression(E) ::= TOKEN_INT(I) TOKEN_LPAREN expression(S) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::CastExpr{
            .cast_to = ValueType::Integer,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(I.span, RP.span),
    };
}

expression(E) ::= TOKEN_FLOAT(F) TOKEN_LPAREN expression(S) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::CastExpr{
            .cast_to = ValueType::Floating,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(F.span, RP.span),
    };
}

expression(E) ::= TOKEN_BOOL(B) TOKEN_LPAREN expression(S) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::CastExpr{
            .cast_to = ValueType::Boolean,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(B.span, RP.span),
    };
}

expression(E) ::= TOKEN_EXCLAMATION(Ex) expression(S). {
    E = new ast::Expr{
        .node = ast::UnaryExpr{
            .type = common::ast::UnaryExprType::Not,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(Ex.span, S->span),
    };
}

expression(E) ::= TOKEN_NOT(N) expression(S). {
    E = new ast::Expr{
        .node = ast::UnaryExpr{
            .type = common::ast::UnaryExprType::Not,
            .expr = Box<ast::Expr>(S),
        },
        .span = merge(N.span, S->span),
    };
}

expression(E) ::= TOKEN_COUNT(C) TOKEN_LPAREN expression(X) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.push_back(std::move(*X));
    delete X;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_count_nonnull",
            .args = std::move(args),
        },
        .span = merge(C.span, RP.span),
    };
}

expression(E) ::= TOKEN_COUNT(C) TOKEN_LPAREN TOKEN_STAR TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_count_all",
            .args = {},
        },
        .span = merge(C.span, RP.span),
    };
}

expression(E) ::= TOKEN_MIN(M) TOKEN_LPAREN expression(X) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.push_back(std::move(*X));
    delete X;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_min",
            .args = std::move(args),
        },
        .span = merge(M.span, RP.span),
    };
}

expression(E) ::= TOKEN_MAX(M) TOKEN_LPAREN expression(X) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.push_back(std::move(*X));
    delete X;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_max",
            .args = std::move(args),
        },
        .span = merge(M.span, RP.span),
    };
}

expression(E) ::= TOKEN_SUM(S) TOKEN_LPAREN expression(X) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.push_back(std::move(*X));
    delete X;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_sum",
            .args = std::move(args),
        },
        .span = merge(S.span, RP.span),
    };
}

expression(E) ::= expression(L) TOKEN_LIKE TOKEN_STR(R). {
    auto str = std::string(R.text);

    E = new ast::Expr{
        .node = ast::LikeExpr{
            .expr = Box<ast::Expr>(L),
            .regex = str.substr(1, str.size() - 2),
        },
        .span = merge(L->span, R.span),
    };
}

expression(E) ::= expression(L) TOKEN_EQ expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Equal,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_NEQ expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::NotEqual,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_AND expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::And,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_OR expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Or,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_DIVIDE expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Divide,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_PLUS expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Plus,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= expression(L) TOKEN_MINUS expression(R). {
    E = new ast::Expr{
        .node = ast::BinaryExpr{
            .type = common::ast::BinaryExprType::Minus,
            .left = Box<ast::Expr>(L),
            .right = Box<ast::Expr>(R),
        },
        .span = merge(L->span, R->span),
    };
}

expression(E) ::= TOKEN_COALESCE(C) TOKEN_LPAREN expression_list(L) TOKEN_RPAREN(RP). {
    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_coalesce",
            .args = std::move(*L),
        },
        .span = merge(C.span, RP.span),
    };

    delete L;
}

expression(E) ::= TOKEN_PERCENTILE(Pe) TOKEN_LPAREN expression(X) TOKEN_COMMA value_list(P) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.reserve(P->size() + 1);

    args.push_back(std::move(*X));
    delete X;

    for (auto&& literal : *P) {
        auto span = literal.span;
        args.push_back(ast::Expr{
            .node = ast::LiteralExpr{ .literal = std::move(literal) },
            .span = span,
        });
    }

    delete P;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_percentile",
            .args = std::move(args),
        },
        .span = merge(Pe.span, RP.span),
    };
}

expression(E) ::= TOKEN_RSUBSTR(Rs) TOKEN_LPAREN expression(A) TOKEN_COMMA TOKEN_STR(P) TOKEN_RPAREN(RP). {
    auto args = std::vector<ast::Expr>();
    args.reserve(2);

    args.push_back(std::move(*A));
    args.push_back(ast::Expr{
        .node = ast::LiteralExpr{
            .literal = Literal{
                .type = ValueType::String,
                .value_str = P.text,
                .span = P.span,
            },
        },
        .span = P.span,
    });

    delete A;

    E = new ast::Expr{
        .node = ast::FnCallExpr{
            .func = "builtin_rsubstr",
            .args = std::move(args),
        },
        .span = merge(Rs.span, RP.span),
    };
}

expression(E) ::= expression(L) TOKEN_IN select_source(S). {
    E = new ast::Expr{
        .node = ast::InExpr{
            .expr = Box<ast::Expr>(L),
            .match = Box<ast::Relation>(S),
        },
        .span = merge(L->span, S->span),
    };
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

value(E) ::= TOKEN_STR(S). {
    E = new Literal{
        .type = ValueType::String,
        .value_str = S.text,
        .span = S.span,
    };
}

value(E) ::= TOKEN_INTEGER(I). {
    E = new Literal{
        .type = ValueType::Integer,
        .value_str = I.text,
        .span = I.span,
    };
}

value(E) ::= TOKEN_FLOATING(I). {
    E = new Literal{
        .type = ValueType::Floating,
        .value_str = I.text,
        .span = I.span,
    };
}

value(E) ::= TOKEN_TRUE(S). {
    E = new Literal{
        .type = ValueType::Boolean,
        .value_str = S.text,
        .span = S.span,
    };
}

value(E) ::= TOKEN_FALSE(S). {
    E = new Literal{
        .type = ValueType::Boolean,
        .value_str = S.text,
        .span = S.span,
    };
}

value(E) ::= TOKEN_NULL(N). {
    E = new Literal{
        .type = ValueType::Null,
        .value_str = N.text,
        .span = N.span,
    };
}

%syntax_error {
    pCtx->has_error = true;
    pCtx->error_span = TOKEN.span;
}
