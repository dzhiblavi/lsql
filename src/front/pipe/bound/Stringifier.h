#pragma once

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Pipeline.h"     // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"       // IWYU pragma: keep
#include "front/pipe/bound/Statements.h"

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

namespace lsql::front::pipe::bound {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    std::string print(const Program& p) {
        binding = p.binding;
        auto b = StrBuilder("Program");
        for (auto&& st : p.statements) {
            b.item(print(st));
        }
        return b.render();
    }

    StrBuilder print(const RelationFieldSetPtr& node) {
        return StrBuilder("fields: {}", to_string(node->fields(), *binding));
    }
};

}  // namespace lsql::front::pipe::bound
