#pragma once

#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/fwd/Expr.h"

#include "core/schema/Fields.h"
#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::bound {

struct StarProjector {};

struct IdentifierProjector {
    FieldId field_id;
};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct FilterStage {
    Box<Expr> condition;
};

struct WhereInStage {
    Box<Expr> expr;
    Box<Pipeline> match;
    FieldId match_field_id;
};

struct TakeStage {
    int count;
};

struct SortStage {
    std::vector<Expr> order_list;
    bool desc;
};

struct SelectStage {
    std::vector<Projector> projectors;
};

struct GroupStage {
    std::vector<Projector> projectors;
    std::vector<Projector> group_list;
};

struct Stage {
    StageNode node;
    common::bound::FieldSetNodePtr fields_out;
};

}  // namespace lsql::front::pipe::bound
