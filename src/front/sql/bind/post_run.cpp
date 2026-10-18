#include "front/sql/bind/post_run.h"

#include "front/sql/bound/Expressions.h"
#include "front/sql/bound/Statement.h"

#include "front/common/source/require_at.h"

#include "util/TreeWalker.h"

#include <algorithm>

namespace lsql::front::sql::bind {

namespace {

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

    void visit(bound::LeftJoinRelation& join) {
        Base::visit(join);

        auto left_fields = join.left->fields_out->fields();
        auto right_fields = join.right->fields_out->fields();
        for (auto id : left_fields.fieldIds()) {
            requireAt(
                !right_fields.contains(id),
                join.right->span,
                "LEFT JOIN inputs have overlapping field '{}'; use aliases or explicit projections",
                binding->name(id));
        }

        auto fits = [](const bound::Expr& key, const FieldSet& fields) {
            return std::ranges::all_of(
                key.required_fields.fieldIds(), [&](auto id) { return fields.contains(id); });
        };
        auto forward = fits(*join.left_key, left_fields) && fits(*join.right_key, right_fields);
        auto reverse = fits(*join.right_key, left_fields) && fits(*join.left_key, right_fields);
        requireAt(
            forward || reverse,
            merge(join.left_key->span, join.right_key->span),
            "LEFT JOIN keys must each reference only their own input");

        if (!forward) {
            std::swap(join.left_key, join.right_key);
        }
    }

    ConstFieldBindingPtr binding;
};

}  // namespace

void postRun(bound::Program& program) {
    PostrunWalker{.binding = program.binding}.visit(program);
}

}  // namespace lsql::front::sql::bind
