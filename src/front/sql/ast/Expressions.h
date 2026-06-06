#pragma once

#include "front/sql/ast/fwd/Expr.h"
#include "front/sql/ast/fwd/Relation.h"

#include "front/common/ast/Expressions.h"
#include "front/common/ast/Literal.h"

#include "core/ValueType.h"
#include "core/types.h"

#include <string>
#include <vector>

namespace lsql::front::sql::ast {

struct IdentifierExpr {
    std::string identifier;
};

struct LiteralExpr {
    common::ast::Literal literal;
};

struct CastExpr {
    ValueType cast_to;
    Box<Expr> expr;
};

struct InExpr {
    Box<Expr> expr;
    Box<Relation> match;
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;
};

struct FnCallExpr {
    std::string func;
    std::vector<Expr> args;
};

struct BinaryExpr {
    common::ast::BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

struct UnaryExpr {
    common::ast::UnaryExprType type;
    Box<Expr> expr;
};

}  // namespace lsql::front::sql::ast
