#pragma once

#include "front/common/ast/Expressions.h"
#include "front/common/ast/Literal.h"

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
    common::ast::Literal literal;
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

struct BinaryExpr {
    common::ast::BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

struct UnaryExpr {
    common::ast::UnaryExprType type;
    Box<Expr> expr;
};

}  // namespace lsql::front::pipe::ast
