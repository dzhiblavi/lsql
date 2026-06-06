#pragma once

namespace lsql::front::common::ast {

enum class BinaryExprType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
    Plus,
    Minus,
};

enum class UnaryExprType {
    Not,
};

}  // namespace lsql::front::common::ast
