#pragma once

#include "interface/sql/plan/FileSourceFunc.h"

#include "interface/sql/ast/BinExpression.h"
#include "interface/sql/ast/FileReference.h"
#include "interface/sql/ast/Program.h"
#include "interface/sql/ast/SelectStatement.h"
#include "interface/sql/ast/UnaryAggregateExpression.h"
#include "interface/sql/ast/UnaryExpression.h"
#include "interface/sql/ast/Visitor.h"

#include "exec/expr/BinaryExpression.h"
#include "exec/expr/Coalesce.h"
#include "exec/expr/Expression.h"
#include "exec/expr/IdentifierExpression.h"
#include "exec/expr/UnaryAggregateExpression.h"
#include "exec/expr/UnaryExpression.h"
#include "exec/expr/ValueExpression.h"

#include "exec/op/Aggregate.h"
#include "exec/op/Filter.h"
#include "exec/op/Group.h"
#include "exec/op/In.h"
#include "exec/op/Limit.h"
#include "exec/op/Materialize.h"
#include "exec/op/MergeSorted.h"
#include "exec/op/Projection.h"
#include "exec/op/Sort.h"
#include "exec/op/UnionAll.h"
#include "exec/op/Values.h"

#include <memory>
#include <stack>
#include <vector>

namespace lsql::sql::plan {

class ExecVisitor : public ast::Visitor {
 public:
    explicit ExecVisitor(GetFileSourceFuncType get_file_source_func)
        : get_file_source_func_(std::move(get_file_source_func)) {}

    void visit(const ast::FileReference& node) override {
        auto op = get_file_source_func_(node.path, std::nullopt);
        sources.push_back(op);
        pushOperation(op);
    }

    void visit(const ast::FileIntervalReference& node) override {
        auto op = get_file_source_func_(
            node.path,
            TimeRange{
                .ts_from = node.ts_from,
                .interval_s = node.interval,
            });

        sources.push_back(op);
        pushOperation(op);
    }

    void visit(const ast::Limit& node) override {
        node.source->visit(*this);
        pushOperation(exec::limit(popOperation(), node.limit));
    }

    void visit(const ast::SelectStatement& node) override {
        node.source->visit(*this);
        auto list = projectorsList(*node.select_list);

        if (std::ranges::any_of(*node.select_list, [](const std::unique_ptr<ast::SelectItem>& x) {
                return x->expr && x->expr->type() == ast::ExpressionType::Group;
            })) {
            auto op = exec::aggregate(popOperation(), std::move(list));
            sources.push_back(op);
            pushOperation(op);
        } else {
            pushOperation(exec::projection(popOperation(), std::move(list)));
        }
    }

    void visit(const ast::GroupBySelect& node) override {
        node.source->visit(*this);
        pushOperation(
            exec::group(
                popOperation(),
                projectorsList(*node.group_list),
                projectorsList(*node.select_list)));
    }

    void visit(const ast::OrderBySelect& node) override {
        node.source->visit(*this);
        pushOperation(
            exec::sort(
                popOperation(), expressionList(*node.order_by->order_list), node.order_by->desc));
    }

    void visit(const ast::UnionAll& node) override {
        node.l->visit(*this);
        auto l = popOperation();
        node.r->visit(*this);
        auto r = popOperation();

        pushOperation(exec::unionAll(l, r));
    }

    void visit(const ast::UnionAllSortedBy& node) override {
        node.l->visit(*this);
        auto l = popOperation();
        node.r->visit(*this);
        auto r = popOperation();

        auto slist = expressionList(*node.slist);
        pushOperation(exec::mergeSorted(l, r, std::move(slist), node.desc));
    }

    void visit(const ast::AdhocRelation& node) override {
        std::vector<Value> values;
        values.reserve(node.values->size());
        for (auto&& node : *node.values) {
            values.push_back(getExpression(*node)->get());
        }

        auto op = exec::values(std::move(values));
        sources.push_back(op);
        pushOperation(op);
    }

    void visit(const ast::BinaryExpression& e) override {
        e.l->visit(*this);
        e.r->visit(*this);
        auto r = popExpression();
        auto l = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.bin_type) {
                case ast::BinExpressionType::Equal:
                    return std::make_shared<exec::BinaryExpression<exec::EqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case ast::BinExpressionType::NotEqual:
                    return std::make_shared<exec::BinaryExpression<exec::NotEqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case ast::BinExpressionType::And:
                    return std::make_shared<exec::BinaryExpression<exec::AndOp>>(l, r);
                case ast::BinExpressionType::Or:
                    return std::make_shared<exec::BinaryExpression<exec::OrOp>>(l, r);
                case ast::BinExpressionType::Divide:
                    return std::make_shared<exec::BinaryExpression<exec::DivideOp>>(
                        l, r, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const ast::UnaryExpression& e) override {
        e.a->visit(*this);
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.un_type) {
                case ast::UnaryExpressionType::BooleanNegate:
                    return std::make_shared<exec::UnaryExpression<exec::BooleanNegationOp>>(arg);
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const ast::LikeExpression& e) override {
        e.a->visit(*this);
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::LikeOp>>(popExpression(), e.regex));
    }

    void visit(const ast::RSubstrExpression& e) override {
        e.arg->visit(*this);
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::RSubstrOp>>(popExpression(), e.regex));
    }

    void visit(const ast::CoalesceExpression& e) override {
        exprs.push_back(std::make_shared<exec::Coalesce>(expressionList(*e.values)));
    }

    void visit(const ast::IdentifierExpression& e) override {
        exprs.push_back(std::make_shared<exec::IdentifierExpression>(e.id));
    }

    void visit(const ast::ValueExpression& e) override { exprs.push_back(getExpression(e)); }

    std::shared_ptr<exec::ValueExpression> getExpression(const ast::ValueExpression& e) {
        switch (e.valueType()) {
            case ValueType::String:
                return std::make_shared<exec::ValueExpression>(
                    e.value_str.substr(1, e.value_str.size() - 2));
            case ValueType::Integer:
                return std::make_shared<exec::ValueExpression>(
                    int64_t(std::atoll(e.value_str.data())));
            case ValueType::Boolean:
                return std::make_shared<exec::ValueExpression>(e.value_str == "true");
            case ValueType::Floating:
                return std::make_shared<exec::ValueExpression>(
                    std::strtof(e.value_str.data(), nullptr));
            case ValueType::Null:
                return std::make_shared<exec::ValueExpression>(Value());
        }
    }

    void visit(const ast::CastExpression& e) override {
        e.expr->visit(*this);
        auto arg = popExpression();

        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::CastOp>>(
                arg, arg->valueType(), e.valueType()));
    }

    void visit(const ast::UnaryAggregateExpression& e) override {
        e.condition->visit(*this);
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case ast::UnaryAggregateType::Count:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::CountOp>>(arg);
                case ast::UnaryAggregateType::Min:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MinOp>>(
                        arg, e.valueType());
                case ast::UnaryAggregateType::Max:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MaxOp>>(
                        arg, e.valueType());
                case ast::UnaryAggregateType::Sum:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::SumOp>>(
                        arg, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const ast::Where& w) override {
        w.source->visit(*this);
        w.expr->visit(*this);

        if (exprs.empty()) {
            return;
        }

        pushOperation(exec::filter(popOperation(), popExpression()));
    }

    void visit(const ast::InExpression& e) override {
        auto source = popOperation();

        e.left->visit(*this);
        auto left = popExpression();
        e.source->visit(*this);
        auto match = popOperation();

        pushOperation(exec::in(source, match, left));
    }

    void visit(const ast::PercentileExpression& e) override {
        e.value->visit(*this);

        exprs.push_back(
            std::make_shared<exec::UnaryAggregateExpression<exec::PercentileOp>>(
                popExpression(), *e.percentiles, e.value->valueType()));
    }

    void visit(const ast::SelectItem& e) override {
        if (e.expr == nullptr) {  // check *
            projectors.push_back(std::make_unique<exec::Projector>(e.name, nullptr));
        } else {
            e.expr->visit(*this);
            projectors.push_back(std::make_unique<exec::Projector>(e.name, popExpression()));
        }
    }

    void visit(const ast::NamedRelation& e) override {
        e.relation->visit(*this);
        named_ops[e.name] = popOperation();
    }

    void visit(const ast::NamedRelationReference& e) override {
        if (!named_ops.contains(e.name)) {
            throw std::runtime_error(std::format("no such named relation {}", e.name));
        }

        pushOperation(named_ops[e.name]);
    }

    void visit(const ast::MaterializedRelation& e) override {
        e.relation->visit(*this);
        auto op = exec::materialize(popOperation());
        pushOperation(op);
        sources.push_back(op);
    }

    void visit(const ast::Program& e) override {
        for (auto&& statement : *e.statements) {
            statement->visit(*this);
        }
    }

    std::shared_ptr<exec::Expression> popExpression() {
        auto top = exprs.back();
        exprs.pop_back();
        return top;
    }

    exec::OperationPtr popOperation() {
        verify(!operations.empty());
        auto top = operations.top();
        operations.pop();
        return top;
    }

    std::vector<exec::ExpressionPtr> expressionList(const ast::ExpressionList& list) {
        verify(exprs.empty());
        for (auto&& item : list) {
            item->visit(*this);
        }
        verify(exprs.size() == list.size());
        return std::move(exprs);
    }

    exec::ProjectionList projectorsList(const ast::SelectList& list) {
        verify(projectors.empty());
        for (auto&& item : list) {
            item->visit(*this);
        }
        verify(projectors.size() == list.size());
        return std::move(projectors);
    }

    void pushOperation(exec::OperationPtr op) { operations.push(op); }

    auto result() && { return std::make_pair(std::move(sources), std::move(operations)); }

 private:
    GetFileSourceFuncType get_file_source_func_;

    std::unordered_map<std::string, exec::OperationPtr> named_ops;
    exec::ProjectionList projectors;
    std::vector<exec::ExpressionPtr> exprs;

    std::vector<exec::SourcePtr> sources;
    std::stack<exec::OperationPtr> operations;
};

}  // namespace lsql::sql::plan
