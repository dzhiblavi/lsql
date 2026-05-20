#pragma once

#include "interface/sql/ast/Expr.h"

#include <variant>

namespace lsql::iface::sql::bind {

struct FieldExpr;
struct ValueExpr;
struct InExpr;
struct CoalesceExpr;
struct CastExpr;
struct PercentileExpr;
struct LikeExpr;
struct RSubstrExpr;
struct UnaryExpr;
struct UnaryAggregateExpr;
struct BinaryExpr;

using Expr = std::variant< //
    FieldExpr,
    ValueExpr, //
    InExpr, //
    CoalesceExpr, //
    CastExpr, //
    PercentileExpr, //
    LikeExpr, //
    RSubstrExpr, //
    UnaryExpr, //
    UnaryAggregateExpr, //
    BinaryExpr //
>;

}  // namespace lsql::iface::sql::bind
