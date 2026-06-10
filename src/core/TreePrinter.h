#pragma once

#include "core/Fields.h"
#include "core/Value.h"

#include "util/TreePrinter.h"

namespace lsql {

template <typename Self>
struct TreePrinter : util::TreePrinter<Self> {
    using StrBuilder = util::StrBuilder;
    using util::TreePrinter<Self>::print;

    StrBuilder print(FieldId s) { return to_string(s, *binding); }
    StrBuilder print(const FieldSet& s) { return to_string(s, *binding); }
    StrBuilder print(const Value& v) { return to_string(v); }

    ConstFieldBindingPtr binding;
};

}  // namespace lsql
