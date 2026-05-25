#pragma once

#include <stdexcept>

namespace lsql {

enum class ExprKindLevel {
    Const,
    Row,
    Group,
};

inline bool composable(ExprKindLevel a, ExprKindLevel b) {
    return a == b || a == ExprKindLevel::Const || b == ExprKindLevel::Const;
}

inline ExprKindLevel composed(ExprKindLevel a, ExprKindLevel b) {
    if (!composable(a, b)) {
        throw std::runtime_error("incompatible expr levels");
    }

    if (a == b) {
        return a;
    }

    return a == ExprKindLevel::Const ? b : a;
}

}  // namespace lsql
