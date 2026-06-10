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

    StrBuilder print(FieldSetNodePtr node) {
        return StrBuilder()
            .block(StrBuilder("this: {}", to_string(node->fieldSet(), *binding)))
            .block(StrBuilder("subtree: {}", to_string(node->subtreeFieldSet(), *binding)));
    }
};

}  // namespace lsql::front::sql::bound
