#pragma once

#include "front/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "front/sql/ast/Relations.h"    // IWYU pragma: keep
#include "front/sql/ast/Statement.h"    // IWYU pragma: keep

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

namespace lsql::front::sql::ast {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    std::string print(const Program& program) {
        auto b = StrBuilder("Program AST");
        for (auto&& s : program) {
            b.item(print(s));
        }
        return b.render();
    }

    StrBuilder print(const SourceSpan& span) {
        return std::format(
            "{}:{}-{}:{}", span.begin.line, span.begin.column, span.end.line, span.end.column);
    }
};

}  // namespace lsql::front::sql::ast
