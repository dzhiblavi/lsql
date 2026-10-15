#pragma once

#include "front/sql/bound/Expressions.h"
#include "front/sql/bound/Statement.h"

#include "front/common/source/require_at.h"

#include "util/TreeWalker.h"

namespace lsql::front::sql::bind {

struct PostrunWalker : util::TreeWalker<PostrunWalker> {
    using Base = util::TreeWalker<PostrunWalker>;
    using Base::visit;

    void visit(bound::InExpr& in) {
        Base::visit(in);

        auto match_fields = in.match->fields_out->fields();
        auto count = match_fields.size();
        requireAt(count == 1, in.match->span, "in match should contain one column, got {}", count);

        auto match_field = *match_fields.fieldIds().begin();
        requireAt(
            in.expr->value_type == binding->type(match_field),
            in.expr->span,
            "in key type mismatch");

        in.match_field_id = match_field;
    }

    ConstFieldBindingPtr binding;
};

void postRun(bound::Program& program) {
    PostrunWalker{.binding = program.binding}.visit(program);
}

}  // namespace lsql::front::sql::bind
