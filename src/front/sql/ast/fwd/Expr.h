#pragma once

#include <variant>

namespace lsql::front::sql::ast {

struct IdentifierExpr;
struct LiteralExpr;
struct CastExpr;
struct InExpr;
struct LikeExpr;
struct FnCallExpr;
struct BinaryExpr;
struct UnaryExpr;

using ExprNode = std::variant< //
    IdentifierExpr, //
    LiteralExpr, //
    CastExpr, //
    InExpr, //
    FnCallExpr, //
    LikeExpr, //
    BinaryExpr, //
    UnaryExpr //
>;

struct Expr;

}  // namespace lsql::front::sql::ast
