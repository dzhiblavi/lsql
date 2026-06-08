#pragma once

#include <variant>

namespace lsql::front::pipe::ast {

struct AdhocSource;
struct NamedPipelineReferenceSource;
struct FileSource;
struct FileIntervalSource;
struct UnionAllSource;
struct UnionAllSortedBySource;

using SourceNode = std::variant< //
    AdhocSource, //
    NamedPipelineReferenceSource, //
    FileSource, //
    FileIntervalSource, //
    UnionAllSource, //
    UnionAllSortedBySource //
>;

struct Source;

}  // namespace lsql::front::pipe::ast
