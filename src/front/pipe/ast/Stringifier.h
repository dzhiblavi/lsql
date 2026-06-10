#pragma once

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Sources.h"      // IWYU pragma: keep
#include "front/pipe/ast/Stages.h"       // IWYU pragma: keep
#include "front/pipe/ast/Statements.h"

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

namespace lsql::front::pipe::ast {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    std::string print(const Program& p) {
        auto b = StrBuilder("Program");
        for (auto&& st : p.statements) {
            b.item(print(st));
        }
        return b.render();
    }

    StrBuilder print(const SourceSpan& span) {
        return std::format(
            "{}:{}-{}:{}", span.begin.line, span.begin.column, span.end.line, span.end.column);
    }
};

}  // namespace lsql::front::pipe::ast
