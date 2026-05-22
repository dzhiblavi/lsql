#pragma once

#include <variant>

namespace lsql::iface::sql::bind {

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

}  // namespace lsql::iface::sql::bind
