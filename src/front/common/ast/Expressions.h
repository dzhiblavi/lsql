#pragma once

namespace lsql::front::common::ast {

enum class BinaryExprType {
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
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
