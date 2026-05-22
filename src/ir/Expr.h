#pragma once

#include <variant>

namespace lsql::ir {

struct FieldExpr;
struct ValueExpr;
struct CoalesceExpr;
struct CastExpr;
struct PercentileExpr;
struct LikeExpr;
struct RSubstrExpr;
struct UnaryExpr;
struct UnaryAggregateExpr;
struct BinaryExpr;

using ExprNode = std::variant< //
    FieldExpr,
    ValueExpr, //
    CoalesceExpr, //
    CastExpr, //
    PercentileExpr, //
    LikeExpr, //
    RSubstrExpr, //
    UnaryExpr, //
    UnaryAggregateExpr, //
    BinaryExpr //
>;

struct Expr;

}  // namespace lsql::ir
