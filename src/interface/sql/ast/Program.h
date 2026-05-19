#pragma once

#include "interface/sql/ast/Node.h"
#include "interface/sql/ast/Visitor.h"

#include <memory>
#include <vector>

namespace lsql::iface::sql::ast {

class NamedRelation : public Node {
 public:
    NamedRelation(std::string name, std::unique_ptr<Node> relation)
        : name(std::move(name))
        , relation(std::move(relation)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string name;
    std::unique_ptr<Node> relation;
};

class NamedRelationReference : public Node {
 public:
    explicit NamedRelationReference(std::string name) : name(std::move(name)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::string name;
};

class MaterializedRelation : public Node {
 public:
    explicit MaterializedRelation(std::unique_ptr<Node> relation) : relation(std::move(relation)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Node> relation;
};

class UnionAll : public Node {
 public:
    UnionAll(std::unique_ptr<Node> l, std::unique_ptr<Node> r) : l(std::move(l)), r(std::move(r)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Node> l;
    std::unique_ptr<Node> r;
};

class Program : public Node {
 public:
    explicit Program(std::vector<std::unique_ptr<Node>>* statements) : statements(statements) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<std::vector<std::unique_ptr<Node>>> statements;
};

}  // namespace lsql::iface::sql::ast
