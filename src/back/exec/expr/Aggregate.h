#pragma once

#include "back/exec/Record.h"

#include "core/schema/Fields.h"
#include "core/types.h"
#include "core/value/Value.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

namespace lsql::back::exec {

class Aggregator {
 public:
    virtual ~Aggregator() = default;

    virtual Value get() = 0;  // one-shot
    virtual void feed(const back::exec::Record& record) = 0;
};

using AggregatorPtr = Arc<Aggregator>;

class Aggregate {
 public:
    virtual ~Aggregate() = default;

    virtual FieldSet requiredFields() const = 0;
    virtual ValueType valueType() const = 0;

    virtual AggregatorPtr aggregator() const = 0;
};

using AggregatePtr = Arc<Aggregate>;

}  // namespace lsql::back::exec
