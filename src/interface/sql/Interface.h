#pragma once

#include "interface/Interface.h"

#include <string>

namespace lsql::iface::sql {

struct TimeRange {
    std::string ts_from;
    int interval_s;
};

using GetFileSourceFuncType = std::function<exec::SourcePtr(std::string, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

class Interface : public iface::Interface {
 public:
    Interface(std::istream* query_in, GetFileSourceFuncType get_file_source_func)
        : query_in_(query_in)
        , get_file_source_func_(std::move(get_file_source_func)) {}

    Plan plan() const override;

 private:
    std::istream* query_in_;
    GetFileSourceFuncType get_file_source_func_;
};

}  // namespace lsql::iface::sql
