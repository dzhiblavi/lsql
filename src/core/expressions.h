#pragma once

namespace lsql {

enum class UnaryExprType {
    BooleanNegate,
};

enum class UnaryAggregateType {
    Count,
    Min,
    Max,
    Sum,
};

enum class BinaryExprType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
    Add,
    Subtract,
};

}  // namespace lsql
