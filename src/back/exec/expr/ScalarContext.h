#pragma once

#include "core/value/Value.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace lsql::back::exec {

class ScalarContext {
 public:
    using Id = size_t;

    Id addSlot() {
        auto id = values_.size();
        values_.emplace_back();
        return id;
    }

    std::optional<Value>& value(Id id) { return values_.at(id); }

    void reset() {
        for (auto& value : values_) {
            value.reset();
        }
    }

 private:
    std::vector<std::optional<Value>> values_;
};

}  // namespace lsql::back::exec
