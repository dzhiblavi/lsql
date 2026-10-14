#pragma once

#include "front/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "front/sql/bound/Relations.h"    // IWYU pragma: keep
#include "front/sql/bound/Statement.h"    // IWYU pragma: keep

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

namespace lsql::front::sql::bound {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    std::string print(const Program& program) {
        auto b = StrBuilder("Bound AST");
        binding = program.binding;
        for (auto&& s : program.statements) {
            b.item(print(s));
        }
        return b.render();
    }

    StrBuilder print(RelationFieldSetPtr node) {
        return StrBuilder("fields: {}", to_string(node->fields(), *binding));
    }
};

}  // namespace lsql::front::sql::bound
