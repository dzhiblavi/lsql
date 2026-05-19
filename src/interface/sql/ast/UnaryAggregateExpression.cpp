#include "interface/sql/ast/UnaryAggregateExpression.h"

#include "core/ValueType.h"

namespace lsql::sql::ast {

ValueType unaryAggregateResultType(UnaryAggregateType type, ValueType a) {
    switch (type) {
        case UnaryAggregateType::Count:
            return ValueType::Integer;
        case UnaryAggregateType::Min:
            return a;
        case UnaryAggregateType::Max:
            return a;
        case UnaryAggregateType::Sum:
            return a;
    }
}

}  // namespace lsql::sql::ast
