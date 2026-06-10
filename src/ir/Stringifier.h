#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"   // IWYU pragma: keep

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

#include <magic_enum/magic_enum.hpp>
#include <rfl.hpp>

namespace lsql::ir {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    util::StrBuilder print(const Program& program) {
        binding = program.field_binding;
        util::StrBuilder b("Program IR");

        for (auto&& s : program.statements) {
            b.item(print(s));
        }

        return b.render();
    }
};

}  // namespace lsql::ir
