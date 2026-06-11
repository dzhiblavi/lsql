#pragma once

#include "back/exec/Record.h"
#include "back/exec/op/Aggregate.h"
#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/Projection.h"

#include "core/types.h"
#include "util/verify.h"

#include <vector>

namespace lsql::back::exec {

using GroupValues = absl::flat_hash_map<FieldId, Value>;

class GroupRecord : public Record {
 public:
    explicit GroupRecord(GroupValues values) : values_(std::move(values)) {}

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

 public:
    Group(
        OperationPtr source,
        AggregateProjectionList aggregators,
        ScalarProjectionList group_key,
        ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , aggregators_(toProjectionMap(std::move(aggregators)))
        , group_key_(toProjectionMap(std::move(group_key))) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

 private:
    bool consume(int phase, const Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            verify_dbg(groups_.empty());
        }

        if (record != nullptr) {
            auto key = getKey(*record);

            auto&& aggregators = groups_[key];
            if (aggregators.empty()) {
                prepareAggregators(phase, aggregators);
            }

            for (auto&& [_, aggregator] : aggregators) {
                aggregator->feed(*record);
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
            auto key = std::move(node.key());
            auto group_kv = valuesFromKey(std::move(key));

            GroupValues values;
            values.reserve(required_fields.size());

            for (auto&& [id, value] : group_kv) {
                // insert only required group keys
                if (required_fields.contains(id)) {
                    values.emplace(id, std::move(value));
                }
            }

            // all these aggregators are required by downstream
            auto aggregators = std::move(node.mapped());
            for (auto&& [id, aggregator] : aggregators) {
                values.emplace(id, aggregator->get());
            }

            auto record = arc<GroupRecord>(std::move(values));
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

        // all group key projections are always needed
        for (auto&& [_, proj] : group_key_) {
            upstream.merge(proj->expr->requiredFields());
        }

        for (auto&& id : downstream.fieldIds()) {
            if (group_key_.contains(id)) {
                // already requested
                continue;
            }

            auto it = aggregators_.find(id);
            verify(it != aggregators_.end());
            upstream.merge(it->second->expr->requiredFields());
        }

        return upstream;
    }

    void prepareAggregators(int phase, auto& aggregators) {
        auto&& required_fields = requiredFields(phase);

        // aggregate only required fields
        for (auto&& id : required_fields.fieldIds()) {
            if (group_key_.contains(id)) {
                // comes from group key, no need to calculate additionally
                continue;
            }

            auto it = aggregators_.find(id);
            verify(it != aggregators_.end());
            aggregators.push_back({id, it->second->expr->aggregator()});
        }
    }

    std::vector<Value> getKey(const Record& record) {
        std::vector<Value> key;
        key.reserve(group_key_.size());
        for (auto&& [id, proj] : group_key_) {
            key.push_back(proj->expr->eval(record));
        }
        return key;
    }

    GroupValues valuesFromKey(std::vector<Value> key) {
        GroupValues group_kv;
        group_kv.reserve(group_key_.size());

        size_t group_by_column_index = 0;
        for (auto&& [id, _] : group_key_) {
            group_kv.emplace(id, std::move(key[group_by_column_index++]));
        }

        return group_kv;
    }

    static AggregateProjectionMap toProjectionMap(AggregateProjectionList list) {
        AggregateProjectionMap map;
        map.reserve(list.size());
        for (auto&& proj : list) {
            auto id = proj->field_id;
            map.emplace(id, std::move(proj));
        }
        return map;
    }

    static ScalarProjectionMap toProjectionMap(ScalarProjectionList list) {
        ScalarProjectionMap map;
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
    AggregateProjectionMap aggregators_;
    ScalarProjectionMap group_key_;

    MemberSubscriber<Group> sub_{
        this,
        &Group::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    struct AggregatorField {
        FieldId output_field_id;
        AggregatorPtr aggregator;
    };

    // phase state
    using Groups = std::unordered_map<std::vector<Value>, std::vector<AggregatorField>>;

    int curr_phase_ = 0;
    Groups groups_;
};

OperationPtr group(
    OperationPtr source,
    AggregateProjectionList aggregators,
    ScalarProjectionList group_key,
    ConstFieldBindingPtr binding) {
    return std::make_shared<Group>(
        std::move(source), std::move(aggregators), std::move(group_key), std::move(binding));
}

}  // namespace lsql::back::exec
