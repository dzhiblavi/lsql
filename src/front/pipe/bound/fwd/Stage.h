#pragma once

#include <variant>

namespace lsql::front::pipe::bound {

struct TakeStage;
struct FilterStage;
struct WhereInStage;
struct SortStage;
struct GroupStage;
struct SelectStage;

using StageNode = std::variant< //
    TakeStage, //
    FilterStage, //
    WhereInStage, //
    SortStage, //
    GroupStage, //
    SelectStage //
>;

struct Stage;

}  // namespace lsql::front::pipe::bound
