#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/Operation.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace lsql::exec {

struct Projector {
    std::string name;
    std::shared_ptr<Expression> expr;
};

using ProjectionList = std::vector<std::unique_ptr<Projector>>;

class ProjectionRecord : public exec::Record {
 public:
    ProjectionRecord(const Record* child, std::shared_ptr<const ProjectionList> projectors)
        : child_(child)
        , projectors_(projectors) {}

    values_t values() const override {
        if (projectors_->empty()) {
            return child_->values();
        }

        values_t values;
        for (auto&& col : *projectors_) {
            values.emplace(col->name, col->expr->eval(*child_));
        }
        return values;
    }

    Value value(std::string_view name) const override {
        if (projectors_->empty()) {
            return child_->value(name);
        }

        auto it = std::ranges::find(*projectors_, name, [](auto&& i) { return i->name; });
        assert(it != projectors_->end());
        return (*it)->expr->eval(*child_);
    }

    exec::ConstRecordPtr clone() const override {
        auto c = child_->clone();
        auto res = std::make_shared<ProjectionRecord>(c.get(), projectors_);
        res->child_pin_ = std::move(c);
        return res;
    }

 private:
    const Record* child_;
    std::shared_ptr<const ProjectionList> projectors_;
    exec::ConstRecordPtr child_pin_ = nullptr;  // clone()
};

class Projection : public Operation, public std::enable_shared_from_this<Projection> {
 public:
    Projection(OperationPtr source, ProjectionList projectors)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , projectors_(std::move(projectors)) {}

 private:
    bool consume(int phase, const exec::Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        ProjectionRecord rec(record, {shared_from_this(), &projectors_});
        return emit(phase, &rec);
    }

    void subscribe(int phase) override { source_->subscribe(phase, &sub_); }

    OperationPtr source_;
    ProjectionList projectors_;
    MemberSubscriber<Projection> sub_{this, &Projection::consume};
};

OperationPtr projection(OperationPtr source, ProjectionList slist) {
    return std::make_shared<Projection>(std::move(source), std::move(slist));
}

}  // namespace lsql::exec
