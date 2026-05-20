#pragma once

#include <variant>

namespace lsql::iface::sql::ast {

struct IdentifierExpr;
struct LiteralExpr;
struct CastExpr;
struct InExpr;
struct LikeExpr;
struct FnCallExpr;
struct BinaryExpr;
struct UnaryExpr;

using Expr = std::variant< //
    IdentifierExpr, //
    LiteralExpr, //
    CastExpr, //
    InExpr, //
    FnCallExpr, //
    LikeExpr, //
    BinaryExpr, //
    UnaryExpr //
>;

}  // namespace lsql::iface::sql::ast
