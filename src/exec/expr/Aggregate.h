#pragma once

#include "exec/Record.h"

#include "core/Fields.h"
#include "core/Value.h"

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

class Aggregate {
 public:
    virtual ~Aggregate() = default;

    virtual FieldSet requiredFields() const = 0;
    virtual ValueType valueType() const = 0;

    virtual AggregatorPtr aggregator() const = 0;
};

using AggregatePtr = std::shared_ptr<Aggregate>;

}  // namespace lsql::exec
