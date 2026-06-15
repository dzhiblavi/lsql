#pragma once

#include "core/schema/Fields.h"
#include "core/value/Value.h"

#include "util/TreePrinter.h"

namespace lsql {

template <typename Self>
struct TreePrinter : util::TreePrinter<Self> {
    using StrBuilder = util::StrBuilder;
    using util::TreePrinter<Self>::print;

    StrBuilder print(FieldId s) { return to_string(s, *binding); }
    StrBuilder print(const Value& v) { return to_string(v); }
    StrBuilder print(const FieldSet& s) { return to_string(s, *binding); }
    StrBuilder print(const Schema& s) { return to_string(s, *binding); }

    ConstFieldBindingPtr binding;
};

}  // namespace lsql
