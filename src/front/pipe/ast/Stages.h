#pragma once

#include "front/pipe/ast/Pipeline.h"
#include "front/pipe/ast/fwd/Expr.h"

#include "core/types.h"

#include <vector>

namespace lsql::front::pipe::ast {

struct StarProjector {};

struct IdentifierProjector {
    std::string identifier;
};

struct ExprProjector {
    std::string alias;
    Box<Expr> expr;
};

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct TakeStage {
    int count;
};

struct FilterStage {
    Box<Expr> condition;
};

struct WhereInStage {
    Box<Expr> expr;
    Box<Pipeline> match;
};

struct SortStage {
    std::vector<Expr> order_list;
    bool desc;
};

struct GroupStage {
    std::vector<Projector> projectors;
    std::vector<Projector> group_list;
};

struct SelectStage {
    std::vector<Projector> projectors;
};

}  // namespace lsql::front::pipe::ast
