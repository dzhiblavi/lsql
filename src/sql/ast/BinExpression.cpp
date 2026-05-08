#include "sql/ast/BinExpression.h"

#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>

namespace lsql::sql::ast {

ValueType binExprResultType(BinExpressionType type, ValueType l, ValueType r) {
    switch (type) {
        case BinExpressionType::Equal:
        case BinExpressionType::NotEqual:
            if (l != r && l != ValueType::Null && r != ValueType::Null) {
                throw std::runtime_error("=/!= expect the same type on both sides (or null)");
            }
            return ValueType::Boolean;

        case BinExpressionType::And:
        case BinExpressionType::Or:
            if (l != ValueType::Boolean || r != ValueType::Boolean) {
                throw std::runtime_error("AND/OR expect Boolean on both sides");
            }
            return ValueType::Boolean;

        case BinExpressionType::Divide:
            if (l != r) {
                throw std::runtime_error(
                    std::format(
                        "can't divide different types {} and {}",
                        magic_enum::enum_name(l),
                        magic_enum::enum_name(r)));
            }
            if (!arithmetic(l)) {
                throw std::runtime_error("can't divide non-arithmetic types");
            }
            return l;
    }
}

}  // namespace lsql::sql::ast
