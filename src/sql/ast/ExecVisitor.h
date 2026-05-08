#pragma once

#include "core/time_formats.h"
#include "exec/SearchTimestamp.h"
#include "exec/expr/BinaryExpression.h"
#include "exec/expr/Coalesce.h"
#include "exec/expr/Expression.h"
#include "exec/expr/IdentifierExpression.h"
#include "exec/expr/UnaryAggregateExpression.h"
#include "exec/expr/UnaryExpression.h"
#include "exec/expr/ValueExpression.h"
#include "logs/LogRelation.h"
#include "rel/Aggregate.h"
#include "rel/Filter.h"
#include "rel/Group.h"
#include "rel/Limit.h"
#include "rel/Projection.h"
#include "rel/Sort.h"
#include "sql/ast/BinExpression.h"
#include "sql/ast/FileReference.h"
#include "sql/ast/Program.h"
#include "sql/ast/SelectStatement.h"
#include "sql/ast/UnaryAggregateExpression.h"
#include "sql/ast/UnaryExpression.h"
#include "sql/ast/Visitor.h"

#include <memory>
#include <stack>
#include <vector>

namespace lsql::sql::ast {

class ExecVisitor : public Visitor {
 public:
    logs::LogType getLogType(const data::PagedFile& file) {
        // TODO: this assumes the file starts with a line
        if (auto type = logs::detectLogType(file.page(0)->data())) {
            return *type;
        }

        throw std::runtime_error("failed to detect log type");
    }

    void visit(const FileReference& node) override {
        auto file = data::NativePagedFile::open(node.path);

        relations.push(
            std::make_shared<logs::LogRelation>(
                std::make_shared<data::PagedLog>(file), getLogType(*file)));
    }

    void visit(const FileIntervalReference& node) override {
        auto from = timestampFromString(node.ts_from, TimeFormat::ISO8601);
        auto to = from + node.interval;

        auto file = data::NativePagedFile::open(node.path);
        auto log_type = getLogType(*file);
        auto time_format = logs::timeFormat(log_type);
        auto from_pos = exec::lowerBoundLine(*file, from, time_format);
        auto to_pos = exec::upperBoundLine(*file, to, time_format);

        if (from_pos == std::string::npos || to_pos <= from_pos) {
            from_pos = to_pos = 0;
        }

        relations.push(
            std::make_shared<logs::LogRelation>(
                std::make_shared<data::PagedLog>(file, from_pos, to_pos), log_type));
    }

    void visit(const Limit& node) override {
        node.source->visit(*this);
        relations.push(rel::executeLimit(node.limit, popRelation()));
    }

    void visit(const SelectStatement& node) override {
        node.source->visit(*this);
        auto list = projectorsList(*node.select_list);

        if (std::ranges::any_of(*node.select_list, [](const std::unique_ptr<ast::SelectItem>& x) {
                return x->expr->type() == ExpressionType::Group;
            })) {
            relations.push(rel::executeAggregate(std::move(list), popRelation()));
        } else {
            relations.push(rel::executeProjection(std::move(list), popRelation()));
        }
    }

    void visit(const Where& w) override {
        w.source->visit(*this);
        w.expr->visit(*this);
        relations.push(rel::executeFilter(popExpression(), popRelation()));
    }

    void visit(const GroupBySelect& node) override {
        node.source->visit(*this);
        relations.push(
            rel::executeGroup(
                projectorsList(*node.group_list),
                projectorsList(*node.select_list),
                popRelation()));
    }

    void visit(const OrderBySelect& node) override {
        node.source->visit(*this);
        relations.push(
            rel::executeSort(
                expressionList(*node.order_by->order_list), node.order_by->desc, popRelation()));
    }

    void visit(const BinaryExpression& e) override {
        e.l->visit(*this);
        e.r->visit(*this);
        auto r = popExpression();
        auto l = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.bin_type) {
                case BinExpressionType::Equal:
                    return std::make_shared<exec::BinaryExpression<exec::EqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case BinExpressionType::NotEqual:
                    return std::make_shared<exec::BinaryExpression<exec::NotEqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case BinExpressionType::And:
                    return std::make_shared<exec::BinaryExpression<exec::AndOp>>(l, r);
                case BinExpressionType::Or:
                    return std::make_shared<exec::BinaryExpression<exec::OrOp>>(l, r);
                case BinExpressionType::Divide:
                    return std::make_shared<exec::BinaryExpression<exec::DivideOp>>(
                        l, r, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const UnaryExpression& e) override {
        e.a->visit(*this);
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.un_type) {
                case UnaryExpressionType::BooleanNegate:
                    return std::make_shared<exec::UnaryExpression<exec::BooleanNegationOp>>(arg);
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const LikeExpression& e) override {
        e.a->visit(*this);
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::LikeOp>>(popExpression(), e.regex));
    }

    void visit(const RSubstrExpression& e) override {
        e.arg->visit(*this);
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::RSubstrOp>>(popExpression(), e.regex));
    }

    void visit(const CoalesceExpression& e) override {
        exprs.push_back(std::make_shared<exec::Coalesce>(expressionList(*e.values)));
    }

    void visit(const IdentifierExpression& e) override {
        exprs.push_back(std::make_shared<exec::IdentifierExpression>(e.id));
    }

    void visit(const ValueExpression& e) override {
        auto expr = [&] -> exec::ExpressionPtr {
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
        }();

        exprs.push_back(expr);
    }

    void visit(const CastExpression& e) override {
        e.expr->visit(*this);
        auto arg = popExpression();

        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::CastOp>>(
                arg, arg->valueType(), e.valueType()));
    }

    void visit(const UnaryAggregateExpression& e) override {
        e.condition->visit(*this);
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case UnaryAggregateType::Count:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::CountOp>>(arg);
                case UnaryAggregateType::Min:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MinOp>>(
                        arg, e.valueType());
                case UnaryAggregateType::Max:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MaxOp>>(
                        arg, e.valueType());
                case UnaryAggregateType::Sum:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::SumOp>>(
                        arg, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void visit(const InExpression& e) override {
        e.left->visit(*this);
        auto left = popExpression();
        e.source->visit(*this);
        auto rel = popRelation();

        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::InOp>>(left, rel, left->valueType()));
    }

    void visit(const PercentileExpression& e) override {
        e.value->visit(*this);

        exprs.push_back(
            std::make_shared<exec::UnaryAggregateExpression<exec::PercentileOp>>(
                popExpression(), *e.percentiles, e.value->valueType()));
    }

    void visit(const SelectItem& e) override {
        e.expr->visit(*this);
        projectors.push_back(std::make_unique<rel::Projector>(e.name, popExpression()));
    }

    void visit(const NamedRelation& e) override {
        e.relation->visit(*this);
        named_relations[e.name] = popRelation();
    }

    void visit(const NamedRelationReference& e) override {
        if (!named_relations.contains(e.name)) {
            throw std::runtime_error(std::format("no such named relation {}", e.name));
        }

        relations.push(named_relations[e.name]);
    }

    void visit(const Program& e) override {
        for (auto&& statement : *e.statements) {
            statement->visit(*this);
        }
    }

    std::shared_ptr<exec::Expression> popExpression() {
        assert(!exprs.empty());
        auto top = exprs.back();
        exprs.pop_back();
        return top;
    }

    rel::RelationPtr popRelation() {
        assert(!relations.empty());
        auto top = relations.top();
        relations.pop();
        return top;
    }

    std::vector<exec::ExpressionPtr> expressionList(const ast::ExpressionList& list) {
        assert(exprs.empty());
        for (auto&& item : list) {
            item->visit(*this);
        }
        assert(exprs.size() == list.size());
        return std::move(exprs);
    }

    std::vector<std::unique_ptr<rel::Projector>> projectorsList(const ast::SelectList& list) {
        assert(projectors.empty());
        for (auto&& item : list) {
            item->visit(*this);
        }
        assert(projectors.size() == list.size());
        return std::move(projectors);
    }

    std::unordered_map<std::string, rel::RelationPtr> named_relations;
    std::vector<std::unique_ptr<rel::Projector>> projectors;
    std::vector<exec::ExpressionPtr> exprs;
    std::stack<rel::RelationPtr> relations;
};

}  // namespace lsql::sql::ast
