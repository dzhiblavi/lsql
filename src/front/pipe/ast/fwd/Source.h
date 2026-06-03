#pragma once

#include <variant>

namespace lsql::front::pipe::ast {

struct AdhocSource;
struct NamedPipelineReferenceSource;
struct FileSource;
struct FileIntervalSource;
struct UnionAllSource;
struct UnionAllSortedBySource;

using Source = std::variant< //
    AdhocSource, //
    NamedPipelineReferenceSource, //
    FileSource, //
    FileIntervalSource, //
    UnionAllSource, //
    UnionAllSortedBySource //
>;

}  // namespace lsql::front::pipe::ast
