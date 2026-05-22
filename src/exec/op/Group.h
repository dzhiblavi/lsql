#pragma once

#include "core/verify.h"
#include "exec/Record.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/Projection.h"

#include <vector>

namespace lsql::exec {

using GroupValues = std::unordered_map<FieldId, Value>;

class GroupEnrichedRecord : public Record {
 public:
    GroupEnrichedRecord(RecordRef child, std::shared_ptr<GroupValues> values)
        : child_(std::move(child))
        , values_(std::move(values)) {
        assert(values_ != nullptr);
    }

    ids_t ids() const override {
        auto ids = get(child_)->ids();
        for (auto&& [k, _] : *values_) {
            ids.insert(k);
        }
        return ids;
    }

    Value value(FieldId id) const override {
        if (auto it = values_->find(id); it != values_->end()) {
            return it->second;
        }
        return get(child_)->value(id);
    }

    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<GroupEnrichedRecord>(pin(child_), values_);
    }

 private:
    RecordRef child_;
    std::shared_ptr<GroupValues> values_;
};

class GroupRecord : public Record {
 public:
    explicit GroupRecord(GroupValues values) : values_(std::move(values)) {}

    ids_t ids() const override {
        ids_t ids;
        for (auto&& [k, _] : values_) {
            ids.insert(k);
        }
        return ids;
    }

    Value value(FieldId id) const override {
        auto it = values_.find(id);
        return it == values_.end() ? null : it->second;
    }

    ConstRecordPtr cloneImpl() const override { return std::make_shared<GroupRecord>(*this); }

 private:
    GroupValues values_;
};

class Group : public OperationBase<Group>, public std::enable_shared_from_this<Group> {
    friend class GroupRecord;

    using ProjectionMap = std::unordered_map<FieldId, std::unique_ptr<Projector>>;

 public:
    Group(
        OperationPtr source,
        ProjectionList glist,
        ProjectionList slist,
        ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , glist_(toProjectionMap(std::move(glist)))
        , slist_(toProjectionMap(std::move(slist))) {
        // remove all glist_ columns from slist_
        for (auto&& [name, _] : glist_) {
            slist_.erase(name);
        }
    }

 private:
    bool consume(int phase, const Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            verify(groups_.empty());
        }

        if (record != nullptr) {
            auto group_kv = std::make_shared<GroupValues>();
            std::vector<Value> key;
            key.reserve(glist_.size());
            for (auto&& [name, proj] : glist_) {
                auto value = proj->expr->eval(*record);
                group_kv->emplace(name, value);
                key.push_back(std::move(value));
            }

            auto&& aggregators = groups_[key];

            if (aggregators.empty()) {
                prepareAggregators(phase, aggregators);
            }

            for (auto&& [_, aggregator] : aggregators) {
                GroupEnrichedRecord enriched_record(record, group_kv);
                aggregator->feed(enriched_record);
            }

            if (!active(phase)) {
                groups_.clear();
                return false;
            }

            return true;
        }

        // end of stream
        auto&& required_fields = requiredFields(phase);

        while (!groups_.empty()) {
            auto node = groups_.extract(groups_.begin());
            auto group_values = std::move(node.key());
            auto aggregators = std::move(node.mapped());

            GroupValues group_kv;
            size_t group_by_column_index = 0;
            for (auto&& [name, _] : glist_) {
                group_kv.emplace(name, std::move(group_values[group_by_column_index++]));
            }

            GroupValues values;

            {
                // pass required group by fields
                for (auto&& [id, value] : group_kv) {
                    if (required_fields.contains(id)) {
                        values.emplace(id, std::move(value));
                    }
                }
            }

            {
                // pass required slist fields
                for (auto&& [id, _] : slist_) {
                    if (required_fields.contains(id)) {
                        values.emplace(id, aggregators[id]->get());
                    }
                }
            }

            auto record = std::make_shared<GroupRecord>(std::move(values));
            if (!emit(phase, record.get())) {
                groups_.clear();
                return false;
            }
        }

        return emit(phase, nullptr);
    }

    // Operation
    void init(int phase, const FieldSet& downstream) override {
        source_->subscribe(phase, &sub_, getFieldSet(downstream));
    }

    FieldSet getFieldSet(const FieldSet& downstream) const {
        FieldSet upstream = FieldSet::emptySet();

        // all group projections are always needed
        for (auto&& [_, proj] : glist_) {
            upstream.merge(proj->expr->requiredFields());
        }

        for (auto&& id : downstream.fieldIds()) {
            if (glist_.contains(id)) {
                // already requested
                continue;
            }

            if (auto it = slist_.find(id); it != slist_.end()) {
                upstream.merge(it->second->expr->requiredFields());
            }
        }

        return upstream;
    }

    void prepareAggregators(int phase, auto& aggregators) {
        auto&& required_fields = requiredFields(phase);

        // aggregate only required fields
        for (auto&& id : required_fields.fieldIds()) {
            if (glist_.contains(id)) {
                // comes from group list, no need to calculate additionally
                continue;
            }

            if (auto it = slist_.find(id); it != slist_.end()) {
                aggregators.emplace(id, it->second->expr->aggregator());
            }
        }
    }

    static ProjectionMap toProjectionMap(ProjectionList list) {
        ProjectionMap map;
        map.reserve(list.size());
        for (auto&& proj : list) {
            auto id = proj->field_id;
            map.emplace(id, std::move(proj));
        }
        return map;
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line(description(ctx.phase)).child(source);
    }

    OperationPtr source_;
    ProjectionMap glist_;
    ProjectionMap slist_;
    MemberSubscriber<Group> sub_{
        this,
        &Group::consume,
        prof_.inputHandle(&sub_),
    };

    // phase state
    using Groups =
        std::unordered_map<std::vector<Value>, std::unordered_map<FieldId, AggregatorPtr>>;

    int curr_phase_ = 0;
    Groups groups_;
};

OperationPtr group(
    OperationPtr source, ProjectionList glist, ProjectionList slist, ConstFieldBindingPtr binding) {
    return std::make_shared<Group>(
        std::move(source), std::move(glist), std::move(slist), std::move(binding));
}

}  // namespace lsql::exec
