#pragma once

#include "front/pipe/ast/Literal.h"
#include "front/pipe/ast/Pipeline.h"
#include "front/pipe/ast/fwd/Expr.h"
#include "front/pipe/ast/fwd/Source.h"  // IWYU pragma: keep

#include <string>
#include <vector>

namespace lsql::front::pipe::ast {

struct AdhocSource {
    std::vector<Literal> literals;
};

struct NamedPipelineReferenceSource {
    std::string name;
};

struct FileSource {
    std::string path;
};

struct FileIntervalSource {
    std::string path;
    std::string ts_from;
    int interval_s;
};

struct UnionAllSource {
    Box<Pipeline> left;
    Box<Pipeline> right;
};

struct UnionAllSortedBySource {
    Box<Pipeline> left;
    Box<Pipeline> right;
    std::vector<Expr> order_list;
    bool desc;
};

}  // namespace lsql::front::pipe::ast
