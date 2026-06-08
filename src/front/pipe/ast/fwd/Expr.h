#pragma once

#include <variant>

namespace lsql::front::pipe::ast {

struct IdentifierExpr;
struct LiteralExpr;
struct LikeExpr;
struct InExpr;
struct FnCallExpr;
struct BinaryExpr;
struct UnaryExpr;

using ExprNode = std::variant< //
    IdentifierExpr, //
    LiteralExpr, //
    InExpr, //
    FnCallExpr, //
    LikeExpr, //
    BinaryExpr, //
    UnaryExpr //
>;

struct Expr;

}  // namespace lsql::front::pipe::ast
