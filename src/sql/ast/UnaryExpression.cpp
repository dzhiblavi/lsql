#include "sql/ast/UnaryExpression.h"

#include <stdexcept>

namespace lsql::sql::ast {

ValueType unaryExprResultType(UnaryExpressionType type, ValueType a) {
    switch (type) {
        case UnaryExpressionType::BooleanNegate:
            if (a != ValueType::Boolean) {
                throw std::runtime_error("only Boolean can be negated");
            }
            return ValueType::Boolean;
    }
}

}  // namespace lsql::sql::ast
