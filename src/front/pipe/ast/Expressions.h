#pragma once

#include "front/Literal.h"
#include "front/pipe/ast/Pipeline.h"
#include "front/pipe/ast/fwd/Expr.h"

#include "core/types.h"

#include <string>
#include <vector>

namespace lsql::front::pipe::ast {

struct IdentifierExpr {
    std::string identifier;  // with a leading dot, e.g. ".timestamp"
};

struct LiteralExpr {
    Literal literal;
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;
};

struct InExpr {
    Box<Expr> expr;
    Box<Pipeline> match;
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
    Plus,
    Minus,
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

}  // namespace lsql::front::pipe::ast
