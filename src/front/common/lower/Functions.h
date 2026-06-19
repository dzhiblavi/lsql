#pragma once

#include "core/function/Function.h"
#include "front/common/bound/BinaryExpr.h"
#include "front/common/bound/UnaryExpr.h"

namespace lsql::front::common::lower {

inline func::Function function(bound::BinaryExprType type, ValueType arg_type) {
    switch (type) {
        case bound::BinaryExprType::Equal:
            return func::Equal{};
        case bound::BinaryExprType::NotEqual:
            return func::NotEqual{};
        case bound::BinaryExprType::And:
            return func::And{};
        case bound::BinaryExprType::Or:
            return func::Or{};
        case bound::BinaryExprType::Divide:
            return func::Divide{.arg_type = arg_type};
        case bound::BinaryExprType::Add:
            return func::Add{.arg_type = arg_type};
        case bound::BinaryExprType::Subtract:
            return func::Subtract{.arg_type = arg_type};
    }
}

inline func::Function function(bound::UnaryExprType type) {
    switch (type) {
        case bound::UnaryExprType::BooleanNegate:
            return func::BooleanNegate{};
    }
}

}  // namespace lsql::front::common::lower
