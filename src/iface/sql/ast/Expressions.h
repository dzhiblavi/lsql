#pragma once

#include "core/ValueType.h"
#include "core/types.h"

#include "iface/sql/ast/Expr.h"
#include "iface/sql/ast/Literal.h"
#include "iface/sql/ast/Relation.h"

#include <string>
#include <vector>

namespace lsql::iface::sql::ast {

struct IdentifierExpr {
    std::string identifier;
};

struct LiteralExpr {
    Literal literal;
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

enum class BinaryExprType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
};

struct BinaryExpr {
    BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

enum class UnaryExprType {
    Not,
};

struct UnaryExpr {
    UnaryExprType type;
    Box<Expr> expr;
};

}  // namespace lsql::iface::sql::ast
