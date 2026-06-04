#pragma once

#include <variant>

namespace lsql::front::sql::bound {

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
struct CountAllExpr;

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
    UnaryAggregateExpr, //
    CountAllExpr //
>;

struct Expr;

}  // namespace lsql::front::sql::bound
