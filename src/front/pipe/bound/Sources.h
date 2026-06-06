#pragma once

#include "front/pipe/bound/Pipeline.h"
#include "front/pipe/bound/fwd/Expr.h"
#include "front/pipe/bound/fwd/Source.h"  // IWYU pragma: keep

#include "core/Value.h"

#include <string>
#include <vector>

namespace lsql::front::pipe::bound {

struct AdhocSource {
    std::vector<Value> values;
    FieldId output_field_id;
};

struct NamedPipelineReferenceSource {
    std::string name;
};

struct FileSource {
    std::string path;
};

struct FileIntervalSource {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
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

struct Source {
    SourceNode node;
    common::bound::FieldSetNodePtr fields_out;
};

}  // namespace lsql::front::pipe::bound
