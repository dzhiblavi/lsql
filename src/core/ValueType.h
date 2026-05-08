#pragma once

namespace lsql {

enum class ValueType {
    Null,
    String,
    Integer,
    Floating,
    Boolean,
};

inline bool arithmetic(ValueType type) {
    switch (type) {
        case ValueType::String:
        case ValueType::Boolean:
        case ValueType::Null:
            return false;

        case ValueType::Integer:
        case ValueType::Floating:
            return true;
    }
}

}  // namespace lsql
