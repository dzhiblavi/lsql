%token_type {lsql::sql::parse::Token}
%default_type {lsql::sql::ast::Node*}
%extra_argument {lsql::sql::parse::Context *pCtx}

%include {
    #include "sql/parser/Token.h"
    #include "sql/parser/grammar/parse.h"

    #include "sql/ast/SelectStatement.h"
    #include "sql/ast/Expression.h"
    #include "sql/ast/BinExpression.h"
    #include "sql/ast/UnaryExpression.h"
    #include "sql/ast/UnaryAggregateExpression.h"
    #include "sql/ast/FileReference.h"
    #include "sql/ast/Program.h"

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <vector>

    namespace ast = lsql::sql::ast;
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
%token TOKEN_LIKE.
%token TOKEN_MATERIALIZE.
%token TOKEN_STRING.
%token TOKEN_INT.
%token TOKEN_FLOAT.
%token TOKEN_BOOL.
%token TOKEN_EXCLAMATION.

// Precedence (from LOWEST to HIGHEST)
%left TOKEN_UNION_ALL.
%left TOKEN_UNION_ALL_SORTED_BY.
%left TOKEN_OR.
%left TOKEN_AND.
%left TOKEN_EQ TOKEN_NEQ.
%left TOKEN_DIVIDE.
%right TOKEN_EXCLAMATION.

%type expression {ast::Expression*}
%type group_expression {ast::Expression*}
%type limit_opt {int}
%type select_list {std::vector<std::unique_ptr<ast::SelectItem>>*}
%type expression_list {std::vector<std::unique_ptr<ast::Expression>>*}
%type select_item {ast::SelectItem*}
%type floating_list {std::vector<float>*}
%type group_select_list {std::vector<std::unique_ptr<ast::SelectItem>>*}
%type group_select_item {ast::SelectItem*}
%type group_by_opt {std::vector<std::unique_ptr<ast::SelectItem>>*}
%type group_by_list {std::vector<std::unique_ptr<ast::SelectItem>>*}
%type order_by_opt {ast::OrderBy*}
%type statement_list {std::vector<std::unique_ptr<ast::Node>>*}
%type value {ast::ValueExpression*}
%type value_list {std::vector<std::unique_ptr<ast::ValueExpression>>*}

%start_symbol input

input ::= statement_list(L) TOKEN_EOF. {
    pCtx->root = std::make_unique<ast::Program>(L);
}

statement_list(S) ::= statement(X). {
    S = new std::vector<std::unique_ptr<ast::Node>>();
    S->emplace_back(X);
}

statement_list(S) ::= statement_list(SL) statement(S1). {
    S = SL;
    S->emplace_back(S1);
}

statement(S) ::= relation(R). {
    S = R;
}

statement(S) ::= TOKEN_IDENTIFIER(Name) TOKEN_EQ select_source(R). {
    S = new ast::NamedRelation(Name.text, std::unique_ptr<ast::Node>(R));
}

// immediate = does not require parentheses in SELECT FROM <here>
immediate_relation(R) ::= adhoc_relation(A). {
    R = A;
}

immediate_relation(R) ::= file_source(F). {
    R = F;
}

immediate_relation(R) ::= TOKEN_DOLLAR TOKEN_IDENTIFIER(Name). {
    R = new ast::NamedRelationReference(Name.text);
}

immediate_relation(R) ::= TOKEN_IDENTIFIER(Name) TOKEN_EQ TOKEN_MATERIALIZE
                          TOKEN_LPAREN relation(S) TOKEN_RPAREN. {
    auto M = new ast::MaterializedRelation(std::unique_ptr<ast::Node>(S));
    R = new ast::NamedRelation(Name.text, std::unique_ptr<ast::Node>(M));
}

// all kinds of relations that can be used on top level
relation(R) ::= immediate_relation(A). {
    R = A;
}

relation(R) ::= select_statement(A). {
    R = A;
}

relation(A) ::= relation(L) TOKEN_UNION_ALL relation(R). {
    A = new ast::UnionAll(std::unique_ptr<ast::Node>(L), std::unique_ptr<ast::Node>(R));
}

relation(A) ::= relation(L) TOKEN_UNION_ALL_SORTED_BY expression_list(S) relation(R). {
    A = new ast::UnionAllSortedBy(
        /* desc = */ false,
        std::unique_ptr<ast::Node>(L),
        std::unique_ptr<ast::Node>(R),
        S
    );
}

relation(A) ::= relation(L) TOKEN_UNION_ALL_SORTED_BY expression_list(S) TOKEN_DESC relation(R). {
    A = new ast::UnionAllSortedBy(
        /* desc = */ true,
        std::unique_ptr<ast::Node>(L),
        std::unique_ptr<ast::Node>(R),
        S
    );
}

adhoc_relation(A) ::= TOKEN_LPAREN value_list(L) TOKEN_RPAREN. {
    A = new ast::AdhocRelation(std::unique_ptr<std::vector<std::unique_ptr<ast::ValueExpression>>>(L));
}

select_statement(A) ::= TOKEN_SELECT select_list(L)
                        TOKEN_FROM select_source(F)
                        where_opt(Wh)
                        group_by_opt(Gr)
                        order_by_opt(Ord)
                        limit_opt(Lim).
{
    A = F;

    if (Wh != nullptr) {
        A = new ast::Where(std::unique_ptr<ast::Node>(Wh), std::unique_ptr<ast::Node>(A));
    }

    if (Gr != nullptr) {
        A = new ast::GroupBySelect(
            Gr,
            L,
            std::unique_ptr<ast::Node>(A)
        );
    } else {
        A = new ast::SelectStatement(L, std::unique_ptr<ast::Node>(A));
    }

    if (Ord != nullptr) {
        A = new ast::OrderBySelect(std::unique_ptr<ast::Node>(A), std::unique_ptr<ast::OrderBy>(Ord));
    }

    if (Lim != -1) {
        A = new ast::Limit(Lim, std::unique_ptr<ast::Node>(A));
    }
}

// all relations in parentheses
select_source(S) ::= TOKEN_LPAREN relation(R) TOKEN_RPAREN. {
    S = R;
}

// immediate relations as is
select_source(S) ::= immediate_relation(R). {
    S = R;
}

file_source(F) ::= TOKEN_PATH(File). {
    F = new ast::FileReference(std::string(File.text));
}

file_source(F) ::= TOKEN_PATH(File) TOKEN_AT TOKEN_TIMESTAMP(Ts) TOKEN_PLUS TOKEN_INTEGER(Interval). {
    F = new ast::FileIntervalReference(
        std::string(File.text),
        std::string(Ts.text),
        std::stoi(Interval.text)
    );
}

where_opt(A) ::= . { A = nullptr; }
where_opt(A) ::= TOKEN_WHERE condition(C). { A = C; }

group_by_opt(G) ::= . { G = nullptr; }
group_by_opt(G) ::= TOKEN_GROUP_BY group_by_list(L). { G = L; }

order_by_opt(G) ::= . { G = nullptr; }
order_by_opt(G) ::= TOKEN_ORDER_BY expression_list(L). { G = new ast::OrderBy(L, false); }
order_by_opt(G) ::= TOKEN_ORDER_BY expression_list(L) TOKEN_ASC. { G = new ast::OrderBy(L, false); }
order_by_opt(G) ::= TOKEN_ORDER_BY expression_list(L) TOKEN_DESC. { G = new ast::OrderBy(L, true); }

condition(C) ::= expression(E). { C = E; }

limit_opt(A) ::= TOKEN_LIMIT TOKEN_INTEGER(N). { A = std::atoi(N.text); }
limit_opt(A) ::= . { A = -1; }

select_list(A) ::= TOKEN_STAR. {
    A = new ast::SelectList();
}

select_list(A) ::= select_item(B). {
    A = new ast::SelectList();
    A->push_back(std::unique_ptr<ast::SelectItem>(B));
}

select_list(A) ::= select_list(B) TOKEN_COMMA select_item(C). {
    A = B;
    A->push_back(std::unique_ptr<ast::SelectItem>(C));
}

select_item(A) ::= TOKEN_IDENTIFIER(Id). {
    A = new ast::SelectItem(
        std::make_unique<ast::IdentifierExpression>(Id.text, lsql::ValueType::String),
        std::string(Id.text)
    );
}

select_item(A) ::= expression(E) TOKEN_AS TOKEN_IDENTIFIER(Id). {
    A = new ast::SelectItem(std::unique_ptr<ast::Expression>(E), std::string(Id.text));
}

group_by_list(A) ::= select_item(B). {
    A = new ast::SelectList();
    A->push_back(std::unique_ptr<ast::SelectItem>(B));
}

group_by_list(A) ::= group_by_list(B) TOKEN_COMMA select_item(C). {
    A = B;
    A->push_back(std::unique_ptr<ast::SelectItem>(C));
}

expression_list(A) ::= expression(B). {
    A = new ast::ExpressionList();
    A->push_back(std::unique_ptr<ast::Expression>(B));
}

expression_list(A) ::= expression_list(B) TOKEN_COMMA expression(C). {
    A = B;
    A->push_back(std::unique_ptr<ast::Expression>(C));
}

expression(E) ::= value(V). { E = V; }

expression(E) ::= TOKEN_IDENTIFIER(Id). {
    E = new ast::IdentifierExpression(Id.text, lsql::ValueType::String);
}

expression(E) ::= TOKEN_STRING TOKEN_LPAREN expression(S) TOKEN_RPAREN. {
    E = new ast::CastExpression(std::unique_ptr<ast::Expression>(S), lsql::ValueType::String);
}

expression(E) ::= TOKEN_INT TOKEN_LPAREN expression(S) TOKEN_RPAREN. {
    E = new ast::CastExpression(std::unique_ptr<ast::Expression>(S), lsql::ValueType::Integer);
}

expression(E) ::= TOKEN_FLOAT TOKEN_LPAREN expression(S) TOKEN_RPAREN. {
    E = new ast::CastExpression(std::unique_ptr<ast::Expression>(S), lsql::ValueType::Floating);
}

expression(E) ::= TOKEN_BOOL TOKEN_LPAREN expression(S) TOKEN_RPAREN. {
    E = new ast::CastExpression(std::unique_ptr<ast::Expression>(S), lsql::ValueType::Boolean);
}

expression(E) ::= TOKEN_EXCLAMATION expression(S). {
    E = new ast::UnaryExpression(
        std::unique_ptr<ast::Expression>(S),
        ast::UnaryExpressionType::BooleanNegate
    );
}

expression(E) ::= expression(L) TOKEN_LIKE TOKEN_STR(R). {
    auto str = std::string(R.text);

    E = new ast::LikeExpression(
        std::unique_ptr<ast::Expression>(L),
        str.substr(1, str.size() - 2)
    );
}

expression(E) ::= expression(L) TOKEN_EQ expression(R). {
    E = new ast::BinaryExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Expression>(R),
        ast::BinExpressionType::Equal
    );
}

expression(E) ::= expression(L) TOKEN_NEQ expression(R). {
    E = new ast::BinaryExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Expression>(R),
        ast::BinExpressionType::NotEqual
    );
}

expression(E) ::= expression(L) TOKEN_AND expression(R). {
    E = new ast::BinaryExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Expression>(R),
        ast::BinExpressionType::And
    );
}

expression(E) ::= expression(L) TOKEN_OR expression(R). {
    E = new ast::BinaryExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Expression>(R),
        ast::BinExpressionType::Or
    );
}

expression(E) ::= expression(L) TOKEN_DIVIDE expression(R). {
    E = new ast::BinaryExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Expression>(R),
        ast::BinExpressionType::Divide
    );
}

expression(E) ::= TOKEN_COUNT TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    E = new ast::UnaryAggregateExpression(
        std::unique_ptr<ast::Expression>(X),
        ast::UnaryAggregateType::Count
    );
}

expression(E) ::= TOKEN_MIN TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    E = new ast::UnaryAggregateExpression(
        std::unique_ptr<ast::Expression>(X),
        ast::UnaryAggregateType::Min
    );
}

expression(E) ::= TOKEN_MAX TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    E = new ast::UnaryAggregateExpression(
        std::unique_ptr<ast::Expression>(X),
        ast::UnaryAggregateType::Max
    );
}

expression(E) ::= TOKEN_SUM TOKEN_LPAREN expression(X) TOKEN_RPAREN. {
    E = new ast::UnaryAggregateExpression(
        std::unique_ptr<ast::Expression>(X),
        ast::UnaryAggregateType::Sum
    );
}

expression(E) ::= TOKEN_COALESCE TOKEN_LPAREN expression_list(L) TOKEN_RPAREN. {
    E = new ast::CoalesceExpression(L);
}

expression(E) ::= TOKEN_RSUBSTR TOKEN_LPAREN expression(A) TOKEN_COMMA TOKEN_STR(P) TOKEN_RPAREN. {
    auto str = std::string(P.text);

    E = new ast::RSubstrExpression(
        std::unique_ptr<ast::Expression>(A),
        str.substr(1, str.size() - 2)
    );
}

expression(E) ::= TOKEN_PERCENTILE TOKEN_LPAREN expression(X) TOKEN_COMMA floating_list(P) TOKEN_RPAREN. {
    E = new ast::PercentileExpression(
        std::unique_ptr<ast::Expression>(X),
        P
    );
}

expression(E) ::= expression(L) TOKEN_IN select_source(S). {
    E = new ast::InExpression(
        std::unique_ptr<ast::Expression>(L),
        std::unique_ptr<ast::Node>(S)
    );
}

value_list(L) ::= value(V). {
    L = new std::vector<std::unique_ptr<ast::ValueExpression>>();
    L->emplace_back(V);
}

value_list(L) ::= value_list(L1) TOKEN_COMMA value(V). {
    L = L1;
    L->emplace_back(V);
}

value(E) ::= TOKEN_STR(S). {
    E = new ast::ValueExpression(S.text, lsql::ValueType::String);
}

value(E) ::= TOKEN_INTEGER(I). {
    E = new ast::ValueExpression(I.text, lsql::ValueType::Integer);
}

value(E) ::= TOKEN_FLOATING(I). {
    E = new ast::ValueExpression(I.text, lsql::ValueType::Floating);
}

value(E) ::= TOKEN_TRUE(S). {
    E = new ast::ValueExpression(S.text, lsql::ValueType::Boolean);
}

value(E) ::= TOKEN_FALSE(S). {
    E = new ast::ValueExpression(S.text, lsql::ValueType::Boolean);
}

value(E) ::= TOKEN_NULL. {
    E = new ast::ValueExpression("", lsql::ValueType::Null);
}

floating_list(L) ::= TOKEN_FLOATING(I). {
    L = new std::vector<float>();
    L->push_back(std::strtof(I.text, nullptr));
}

floating_list(L) ::= floating_list(A) TOKEN_COMMA TOKEN_FLOATING(I). {
    L = A;
    L->push_back(std::strtof(I.text, nullptr));
}

%syntax_error {
    const char* token_text = TOKEN.text;
    fprintf(stderr, "Syntax error at line near token: '%s'\n", token_text);
    pCtx->has_error = 1;
}
