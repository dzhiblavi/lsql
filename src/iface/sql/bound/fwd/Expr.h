#pragma once

#include <variant>

namespace lsql::iface::sql::bound {

struct IdentifierExpr;
struct ValueExpr;
struct CastExpr;
struct InExpr;
struct LikeExpr;
struct CoalesceExpr;
struct PercentileExpr;
struct RSubstrExpr;
struct BinaryExpr;
struct UnaryExpr;
struct UnaryAggregateExpr;

using ExprNode = std::variant< //
    IdentifierExpr, //
    ValueExpr, //
    CastExpr, //
    InExpr, //
    LikeExpr, //
    CoalesceExpr, //
    PercentileExpr, //
    RSubstrExpr, //
    BinaryExpr, //
    UnaryExpr, //
    UnaryAggregateExpr //
>;

struct Expr;

}  // namespace lsql::iface::sql::bound
