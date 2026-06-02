#pragma once

#include <variant>

namespace lsql::front::pipe::ast {

struct TakeStage;
struct FilterStage;
struct WhereInStage;
struct SortStage;
struct GroupStage;
struct SelectStage;

using Stage = std::variant< //
    TakeStage, //
    FilterStage, //
    WhereInStage, //
    SortStage, //
    GroupStage, //
    SelectStage //
>;

}  // namespace lsql::front::pipe::ast
