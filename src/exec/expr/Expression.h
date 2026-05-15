#pragma once

#include "core/Value.h"
#include "exec/Record.h"
#include "exec/RequiredFields.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

#include <memory>

namespace lsql::exec {

class Aggregator {
 public:
    virtual ~Aggregator() = default;

    virtual Value get() = 0;  // one-shot
    virtual void feed(const exec::Record& record) = 0;
};

using AggregatorPtr = std::shared_ptr<Aggregator>;

class Expression {
 public:
    virtual ~Expression() = default;

    virtual RequiredFields requiredFields() const = 0;
    virtual ValueType valueType() const = 0;
    virtual AggregatorPtr aggregator() const = 0;
    virtual Value eval(const exec::Record& record) const = 0;
    virtual Value eval(const std::vector<exec::ConstRecordPtr>& group) const = 0;
};

using ExpressionPtr = std::shared_ptr<Expression>;

}  // namespace lsql::exec
