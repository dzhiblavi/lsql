#pragma once

#include "core/schema/Fields.h"
#include "core/value/Value.h"

#include <vector>

namespace lsql::output {

using Record = std::vector<std::pair<FieldId, Value>>;

class Consumer {
 public:
    virtual ~Consumer() = default;

    // can pull values from Record if needed
    virtual void consume(Record& r) = 0;

    virtual void done() = 0;
};

}  // namespace lsql::output
