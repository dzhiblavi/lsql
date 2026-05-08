#pragma once

#include "rel/Record.h"

#include <coro/generator.hpp>

#include <memory>

namespace lsql::rel {

class Relation {
 public:
    virtual ~Relation() = default;
    virtual coro::generator<const Record*> records() const = 0;
};

using RelationPtr = std::shared_ptr<Relation>;

}  // namespace lsql::rel
