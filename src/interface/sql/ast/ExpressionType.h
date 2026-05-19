#pragma once

#include <cassert>
#include <stdexcept>

namespace lsql::iface::sql::ast {

enum class ExpressionType {
    Row,
    Group,
    Const,
};

inline bool composable(ExpressionType a, ExpressionType b) {
    return a == b || a == ExpressionType::Const || b == ExpressionType::Const;
}

inline ExpressionType composed(ExpressionType a, ExpressionType b) {
    if (!composable(a, b)) {
        throw std::runtime_error("cannot compose these expression types");
    }

    if (a == b) {
        return a;
    }

    return a == ExpressionType::Const ? b : a;
}

}  // namespace lsql::iface::sql::ast
