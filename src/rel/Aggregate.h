#pragma once

#include "exec/expr/Expression.h"
#include "rel/Projection.h"
#include "rel/Relation.h"

#include <cassert>
#include <vector>

namespace lsql::rel {

class AggregateRecord : public Record {
 public:
    AggregateRecord(std::shared_ptr<const ProjectionList> projectors, std::vector<Value> values)
        : projectors_(projectors)
        , values_(std::move(values)) {}

    values_t values() const override {
        values_t values;
        for (size_t i = 0; i < projectors_->size(); ++i) {
            values.emplace((*projectors_)[i]->name, values_[i]);
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = std::ranges::find(*projectors_, name, [](auto&& i) { return i->name; });
        assert(it != projectors_->end());
        return values_[it - projectors_->begin()];
    }

    ConstRecordPtr clone() const override { return std::make_shared<AggregateRecord>(*this); }

 private:
    std::shared_ptr<const ProjectionList> projectors_;
    std::vector<Value> values_;
};

class AggregateRelation : public Relation, public std::enable_shared_from_this<AggregateRelation> {
 public:
    AggregateRelation(RelationPtr rel, ProjectionList projectors)
        : rel_(rel)
        , projectors_(std::move(projectors)) {}

    coro::generator<const Record*> records() const override {
        std::vector<exec::AggregatorPtr> aggregators;
        aggregators.reserve(projectors_.size());
        for (auto&& col : projectors_) {
            aggregators.push_back(col->expr->aggregator());
        }

        for (auto* record : rel_->records()) {
            for (size_t i = 0; i < projectors_.size(); ++i) {
                aggregators[i]->feed(*record);
            }
        }

        std::vector<Value> values;
        values.reserve(aggregators.size());
        for (auto&& aggregator : aggregators) {
            values.push_back(aggregator->get());
        }

        AggregateRecord record({shared_from_this(), &projectors_}, std::move(values));
        co_yield &record;
    }

 private:
    RelationPtr rel_;
    ProjectionList projectors_;
};

RelationPtr executeAggregate(ProjectionList slist, RelationPtr rel) {
    return std::make_shared<AggregateRelation>(rel, std::move(slist));
}

}  // namespace lsql::rel
