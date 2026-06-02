#pragma once

#include "back/exec/Record.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>

namespace lsql::back::exec {

class Scalar {
 public:
    virtual ~Scalar() = default;

    virtual FieldSet requiredFields() const = 0;
    virtual ValueType valueType() const = 0;

    virtual Value eval(const back::exec::Record& record) const = 0;
};

using ScalarPtr = Arc<Scalar>;

}  // namespace lsql::back::exec
