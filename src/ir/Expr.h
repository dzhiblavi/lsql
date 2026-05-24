#pragma once

#include <variant>

namespace lsql::ir {

struct FieldExpr;
struct ValueExpr;
struct CoalesceExpr;
struct CastExpr;
struct LikeExpr;
struct RSubstrExpr;
struct UnaryExpr;
struct BinaryExpr;

using ExprNode = std::variant< //
    FieldExpr,
    ValueExpr, //
    CoalesceExpr, //
    CastExpr, //
    LikeExpr, //
    RSubstrExpr, //
    UnaryExpr, //
    BinaryExpr //
>;

struct Expr;

}  // namespace lsql::ir
