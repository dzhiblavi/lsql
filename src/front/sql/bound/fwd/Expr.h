#pragma once

#include "front/common/bound/Expressions.h"

#include <variant>

namespace lsql::front::sql::bound {

using common::bound::ValueExpr;

struct IdentifierExpr;
struct FnCallExpr;
struct InExpr;
struct LikeExpr;
struct BinaryExpr;
struct UnaryExpr;

using ExprNode = std::variant< //
    IdentifierExpr, //
    ValueExpr, //
    FnCallExpr, //
    InExpr, //
    LikeExpr, //
    BinaryExpr, //
    UnaryExpr //
>;

struct Expr;

}  // namespace lsql::front::sql::bound
