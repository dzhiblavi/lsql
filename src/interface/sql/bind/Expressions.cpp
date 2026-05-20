#include "interface/sql/bind/Expressions.h"

#include "interface/sql/bind/Expr.h"
#include "interface/sql/bind/ExprKindLevel.h"

namespace lsql::iface::sql::bind {

ValueType valueTypeOf(const Expr& e) {
    return std::visit([](auto&& e) { return e.valueType(); }, e);
}

ExprKindLevel exprKindLevelOf(const Expr& e) {
    return std::visit([](auto&& e) { return e.level(); }, e);
}

ValueType CoalesceExpr::valueType() const {
    return args.empty() ? ValueType::Null : valueTypeOf(args.front());
}

ExprKindLevel CoalesceExpr::level() const {
    if (args.empty()) {
        return ExprKindLevel::Const;
    }

    auto level = ExprKindLevel::Const;
    for (auto&& arg : args) {
        level = composed(level, exprKindLevelOf(arg));
    }
    return level;
}

ExprKindLevel CastExpr::level() const {
    return exprKindLevelOf(*expr);
}

ExprKindLevel LikeExpr::level() const {
    return exprKindLevelOf(*expr);
}

ExprKindLevel RSubstrExpr::level() const {
    return exprKindLevelOf(*expr);
}

ExprKindLevel UnaryExpr::level() const {
    return exprKindLevelOf(*expr);
}

ExprKindLevel BinaryExpr::level() const {
    return composed(exprKindLevelOf(*left), exprKindLevelOf(*right));
}

}  // namespace lsql::iface::sql::bind
