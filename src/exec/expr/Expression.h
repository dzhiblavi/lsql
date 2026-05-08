#pragma once

#include "core/Value.h"
#include "rel/Record.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

#include <memory>

namespace lsql::exec {

class Aggregator {
 public:
    virtual ~Aggregator() = default;

    virtual Value get() = 0;  // one-shot
    virtual void feed(const rel::Record& record) = 0;
};

using AggregatorPtr = std::shared_ptr<Aggregator>;

class Expression {
 public:
    virtual ~Expression() = default;

    virtual ValueType valueType() const = 0;
    virtual AggregatorPtr aggregator() const = 0;
    virtual Value eval(const rel::Record& record) const = 0;
    virtual Value eval(const std::vector<rel::ConstRecordPtr>& group) const = 0;
};

using ExpressionPtr = std::shared_ptr<Expression>;

}  // namespace lsql::exec
