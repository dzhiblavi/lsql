#pragma once

#include "interface/sql/ast/Expression.h"
#include "interface/sql/ast/Node.h"
#include "interface/sql/ast/Visitor.h"

#include <memory>
#include <string>
#include <vector>

namespace lsql::sql::ast {

class SelectItem : public Node {
 public:
    SelectItem(std::unique_ptr<Expression> expr, std::string name)
        : expr(std::move(expr))
        , name(std::move(name)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expression> expr;
    std::string name;
};

using SelectList = std::vector<std::unique_ptr<SelectItem>>;

class SelectStatement : public Node {
 public:
    SelectStatement(SelectList* select_list, std::unique_ptr<Node> source)
        : select_list(select_list)
        , source(std::move(source)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<SelectList> select_list;
    std::unique_ptr<Node> source;
};

class Limit : public Node {
 public:
    Limit(int limit, std::unique_ptr<Node> source) : limit{limit}, source{std::move(source)} {}
    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    int limit;
    std::unique_ptr<Node> source;
};

class Where : public Node {
 public:
    Where(std::unique_ptr<Node> expr, std::unique_ptr<Node> source)
        : expr{std::move(expr)}
        , source{std::move(source)} {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Node> expr;
    std::unique_ptr<Node> source;
};

class GroupBySelect : public Node {
 public:
    GroupBySelect(SelectList* group_list, SelectList* select_list, std::unique_ptr<Node> source)
        : group_list(group_list)
        , select_list(select_list)
        , source(std::move(source)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<SelectList> group_list;
    std::unique_ptr<SelectList> select_list;
    std::unique_ptr<Node> source;
};

class OrderBy : public Node {
 public:
    OrderBy(ExpressionList* order_list, bool desc) : desc(desc), order_list(order_list) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    bool desc;
    std::unique_ptr<ExpressionList> order_list;
};

class OrderBySelect : public Node {
 public:
    OrderBySelect(std::unique_ptr<Node> source, std::unique_ptr<OrderBy> order_by)
        : source(std::move(source))
        , order_by(std::move(order_by)) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Node> source;
    std::unique_ptr<OrderBy> order_by;
};

class UnionAllSortedBy : public Node {
 public:
    UnionAllSortedBy(
        bool desc, std::unique_ptr<Node> l, std::unique_ptr<Node> r, ExpressionList* slist)
        : desc(desc)
        , l(std::move(l))
        , r(std::move(r))
        , slist(slist) {}

    void visit(Visitor& visitor) const override { visitor.visit(*this); }

    bool desc;
    std::unique_ptr<Node> l;
    std::unique_ptr<Node> r;
    std::unique_ptr<ExpressionList> slist;
};

}  // namespace lsql::sql::ast
