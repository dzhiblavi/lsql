#pragma once

#include "front/common/bound/Expressions.h"

#include <variant>

namespace lsql::front::pipe::bound {

using common::bound::ValueExpr;

struct IdentifierExpr;
struct FnCallExpr;
struct InExpr;
struct LikeExpr;
struct BinaryExpr;
struct UnaryExpr;

using ExprNode = std::variant< //
    IdentifierExpr, //
    FnCallExpr, //
    ValueExpr, //
    InExpr, //
    LikeExpr, //
    BinaryExpr, //
    UnaryExpr //
>;

struct Expr;

}  // namespace lsql::front::pipe::bound
