#pragma once

#include "core/schema/FieldSet.h"
#include "core/types.h"

namespace lsql::front::common::bound {

class RelationFieldSet {
 public:
    virtual ~RelationFieldSet() = default;

    virtual FieldSet fields() const = 0;
    virtual bool canProvide(FieldId id) const = 0;
    virtual void require(FieldId id) = 0;
};

using RelationFieldSetPtr = Arc<RelationFieldSet>;

}
