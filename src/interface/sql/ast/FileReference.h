#pragma once

#include "interface/sql/ast/Node.h"
#include "interface/sql/ast/Visitor.h"

#include <string>

namespace lsql::iface::sql::ast {

class FileReference : public Node {
 public:
    explicit FileReference(std::string path) : path(std::move(path)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string path;
};

class FileIntervalReference : public Node {
 public:
    FileIntervalReference(std::string path, std::string ts_from, int interval)
        : path(std::move(path))
        , ts_from(std::move(ts_from))
        , interval(interval) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string path;
    std::string ts_from;
    int interval;
};

}  // namespace lsql::iface::sql::ast
