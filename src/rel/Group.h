#pragma once

#include "rel/Projection.h"
#include "rel/Record.h"
#include "rel/Relation.h"

#include <vector>

namespace lsql::rel {

using GroupValues = std::unordered_map<std::string_view, Value>;

class GroupEnrichedRecord : public Record,
                            public std::enable_shared_from_this<GroupEnrichedRecord> {
 public:
    GroupEnrichedRecord(ConstRecordPtr child, std::shared_ptr<GroupValues> values)
        : child_(std::move(child))
        , values_(std::move(values)) {}

    values_t values() const override {
        auto values = child_->values();
        for (auto&& [k, v] : *values_) {
            values.emplace(k, v);
        }
        return values;
    }

    Value value(std::string_view name) const override {
        if (auto it = values_->find(name); it != values_->end()) {
            return it->second;
        }
        return child_->value(name);
    }

    std::shared_ptr<const Record> clone() const override { return shared_from_this(); }

 private:
    ConstRecordPtr child_;
    std::shared_ptr<GroupValues> values_;
};

class GroupRecord : public Record {
 public:
    GroupRecord(
        std::shared_ptr<std::vector<ConstRecordPtr>> records,
        std::shared_ptr<const ProjectionList> slist)
        : records_(std::move(records))
        , slist_(slist) {}

    values_t values() const override {
        values_t values;
        for (auto&& key : *slist_) {
            values.emplace(key->name, key->expr->eval(*records_));
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = std::ranges::find(*slist_, name, [](auto&& i) { return i->name; });
        assert(it != slist_->end());
        return (*it)->expr->eval(*records_);
    }

    ConstRecordPtr clone() const override { return std::make_shared<GroupRecord>(*this); }

 private:
    std::shared_ptr<std::vector<ConstRecordPtr>> records_;
    std::shared_ptr<const ProjectionList> slist_;
};

class GroupRelation : public Relation, public std::enable_shared_from_this<GroupRelation> {
    friend class GroupRecord;

 public:
    GroupRelation(RelationPtr rel, ProjectionList glist, ProjectionList slist)
        : rel_(rel)
        , glist_(std::move(glist))
        , slist_(std::move(slist)) {
        if (slist_.empty()) {
            throw std::runtime_error("GROUP select list cannot be empty");
        }
    }

    coro::generator<const Record*> records() const override {
        std::unordered_map<std::vector<Value>, std::shared_ptr<std::vector<ConstRecordPtr>>> groups;

        for (auto* record : rel_->records()) {
            std::vector<Value> key;
            key.reserve(glist_.size());
            for (auto&& col : glist_) {
                key.push_back(col->expr->eval(*record));
            }

            auto it = groups.find(key);
            if (it == groups.end()) {
                it = groups.emplace(std::move(key), std::make_shared<std::vector<ConstRecordPtr>>())
                         .first;
            }

            it->second->push_back(record->clone());
        }

        while (!groups.empty()) {
            auto node = groups.extract(groups.begin());

            auto group_values = std::move(node.key());
            auto group_kv = std::make_shared<GroupValues>();
            for (size_t i = 0; i < glist_.size(); ++i) {
                group_kv->emplace(glist_[i]->name, std::move(group_values[i]));
            }

            auto records = std::make_shared<std::vector<ConstRecordPtr>>();
            records->reserve(node.mapped()->size());
            for (auto&& record : *node.mapped()) {
                records->push_back(std::make_shared<GroupEnrichedRecord>(record, group_kv));
            }

            GroupRecord record(std::move(records), {shared_from_this(), &slist_});
            co_yield &record;
        }
    }

 private:
    RelationPtr rel_;
    ProjectionList glist_;
    ProjectionList slist_;
};

RelationPtr executeGroup(ProjectionList glist, ProjectionList slist, RelationPtr rel) {
    return std::make_shared<GroupRelation>(rel, std::move(glist), std::move(slist));
}

}  // namespace lsql::rel
