#pragma once

namespace lsql::sql::ast {

class Visitor;

class Node {
 public:
    virtual ~Node() = default;
    virtual void visit(Visitor& visitor) const = 0;
};

}  // namespace lsql::sql::ast
