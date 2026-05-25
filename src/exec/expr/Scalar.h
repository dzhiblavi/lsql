#pragma once

#include "exec/Record.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

namespace lsql::exec {

class Scalar {
 public:
    virtual ~Scalar() = default;

    virtual FieldSet requiredFields() const = 0;
    virtual ValueType valueType() const = 0;

    virtual Value eval(const exec::Record& record) const = 0;
};

using ScalarPtr = Arc<Scalar>;

}  // namespace lsql::exec
