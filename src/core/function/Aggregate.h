#pragma once

#include "core/value/Value.h"

#include <span>

namespace lsql::func {

class Aggregator {
 public:
    virtual ~Aggregator() = default;

    virtual Value get() = 0;  // one-shot
    virtual void feed(std::span<Value> values) = 0;
};

class Aggregate {
 public:
    virtual ~Aggregate() = default;

    virtual Box<Aggregator> aggregator() const = 0;
};

}  // namespace lsql::func
