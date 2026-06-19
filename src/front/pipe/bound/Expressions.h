#pragma once

#include "front/common/bound/BinaryExpr.h"
#include "front/common/bound/ExprKindLevel.h"
#include "front/common/bound/UnaryExpr.h"
#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/fwd/Expr.h"

#include "core/function/Function.h"
#include "core/types.h"

#include <string>

namespace lsql::front::pipe::bound {

struct IdentifierExpr {
    FieldId field_id;
};

struct FnCallExpr {
    func::Function func;
    std::vector<Expr> args;
};

struct InExpr {
    Box<Expr> expr;
    Box<Pipeline> match;
    FieldId match_field_id;
};

struct LikeExpr {
    Box<Expr> expr;
    std::string regex;
};

struct BinaryExpr {
    common::bound::BinaryExprType type;
    Box<Expr> left;
    Box<Expr> right;
};

struct UnaryExpr {
    common::bound::UnaryExprType type;
    Box<Expr> expr;
};

struct Expr {
    ExprNode node;
    ValueType value_type;
    common::bound::ExprKindLevel level;
    FieldSet required_fields;
};

}  // namespace lsql::front::pipe::bound
