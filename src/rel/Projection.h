#pragma once

#include "exec/expr/Expression.h"
#include "rel/Relation.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace lsql::rel {

struct Projector {
    std::string name;
    std::shared_ptr<exec::Expression> expr;
};

using ProjectionList = std::vector<std::unique_ptr<Projector>>;

class ProjectionRecord : public Record {
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

    ConstRecordPtr clone() const override {
        auto c = child_->clone();
        auto res = std::make_shared<ProjectionRecord>(c.get(), projectors_);
        res->child_pin_ = std::move(c);
        return res;
    }

 private:
    const Record* child_;
    std::shared_ptr<const ProjectionList> projectors_;
    ConstRecordPtr child_pin_ = nullptr;  // clone()
};

class ProjectionRelation : public Relation,
                           public std::enable_shared_from_this<ProjectionRelation> {
 public:
    ProjectionRelation(RelationPtr rel, ProjectionList projectors)
        : rel_(rel)
        , projectors_(std::move(projectors)) {}

    coro::generator<const Record*> records() const override {
        for (auto* record : rel_->records()) {
            ProjectionRecord rec(record, {shared_from_this(), &projectors_});
            co_yield &rec;
        }
    }

 private:
    RelationPtr rel_;
    ProjectionList projectors_;
};

RelationPtr executeProjection(ProjectionList slist, RelationPtr rel) {
    return std::make_shared<ProjectionRelation>(rel, std::move(slist));
}

}  // namespace lsql::rel
