#pragma once

#include "exec/op/Projection.h"
#include "exec/Record.h"

#include <vector>

namespace lsql::exec {

using GroupValues = std::unordered_map<std::string_view, Value>;

class GroupEnrichedRecord : public exec::Record,
                            public std::enable_shared_from_this<GroupEnrichedRecord> {
 public:
    GroupEnrichedRecord(exec::ConstRecordPtr child, std::shared_ptr<GroupValues> values)
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
    exec::ConstRecordPtr child_;
    std::shared_ptr<GroupValues> values_;
};

class GroupRecord : public exec::Record {
 public:
    GroupRecord(
        std::shared_ptr<std::vector<exec::ConstRecordPtr>> records,
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

    exec::ConstRecordPtr clone() const override { return std::make_shared<GroupRecord>(*this); }

 private:
    std::shared_ptr<std::vector<exec::ConstRecordPtr>> records_;
    std::shared_ptr<const ProjectionList> slist_;
};

class Group : public Operation, public std::enable_shared_from_this<Group> {
    friend class GroupRecord;

 public:
    Group(OperationPtr source, ProjectionList glist, ProjectionList slist)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , glist_(std::move(glist))
        , slist_(std::move(slist)) {
        if (slist_.empty()) {
            throw std::runtime_error("GROUP select list cannot be empty");
        }
    }

 private:
    bool consume(int phase, const exec::Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            assert(groups_.empty());
        }

        if (record != nullptr) {
            std::vector<Value> key;
            key.reserve(glist_.size());
            for (auto&& col : glist_) {
                key.push_back(col->expr->eval(*record));
            }
            auto it = groups_.find(key);
            if (it == groups_.end()) {
                it = groups_
                         .emplace(
                             std::move(key), std::make_shared<std::vector<exec::ConstRecordPtr>>())
                         .first;
            }
            it->second->push_back(record->clone());
            return active(phase);
        }

        // end of stream
        while (!groups_.empty()) {
            auto node = groups_.extract(groups_.begin());

            auto group_values = std::move(node.key());
            auto group_kv = std::make_shared<GroupValues>();
            for (size_t i = 0; i < glist_.size(); ++i) {
                group_kv->emplace(glist_[i]->name, std::move(group_values[i]));
            }

            auto records = std::make_shared<std::vector<exec::ConstRecordPtr>>();
            records->reserve(node.mapped()->size());
            for (auto&& record : *node.mapped()) {
                records->push_back(std::make_shared<GroupEnrichedRecord>(record, group_kv));
            }

            GroupRecord record(std::move(records), {shared_from_this(), &slist_});
            if (!emit(phase, &record)) {
                return false;
            }
        }

        return emit(phase, nullptr);
    }

    void subscribe(int phase) override { source_->subscribe(phase, &sub_); }

    OperationPtr source_;
    ProjectionList glist_;
    ProjectionList slist_;
    MemberSubscriber<Group> sub_{this, &Group::consume};

    // phase state
    using Groups =
        std::unordered_map<std::vector<Value>, std::shared_ptr<std::vector<exec::ConstRecordPtr>>>;

    int curr_phase_ = 0;
    Groups groups_;
};

OperationPtr group(OperationPtr source, ProjectionList glist, ProjectionList slist) {
    return std::make_shared<Group>(std::move(source), std::move(glist), std::move(slist));
}

}  // namespace lsql::exec
