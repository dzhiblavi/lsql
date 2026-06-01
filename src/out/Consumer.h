#pragma once

#include "core/Fields.h"
#include "core/Value.h"

#include <absl/container/flat_hash_map.h>

namespace lsql::out {

using Record = absl::flat_hash_map<FieldId, Value>;

class Consumer {
 public:
    virtual ~Consumer() = default;

    // can pull values from Record if needed
    virtual void consume(Record& r) = 0;

    virtual void done() = 0;
};

}  // namespace lsql::out
