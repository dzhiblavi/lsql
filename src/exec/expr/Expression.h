#pragma once

#include "exec/Record.h"

#include "core/Fields.h"
#include "core/Value.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

#include <memory>

namespace lsql::exec {

class Expression {
 public:
    virtual ~Expression() = default;

    virtual FieldSet requiredFields() const = 0;
    virtual ValueType valueType() const = 0;

    virtual Value eval(const exec::Record& record) const = 0;
};

using ExpressionPtr = std::shared_ptr<Expression>;

}  // namespace lsql::exec
