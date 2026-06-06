#pragma once

#include "front/common/bound/Expressions.h"

#include <variant>

namespace lsql::front::sql::bound {

using common::bound::ValueExpr;

struct IdentifierExpr;
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
