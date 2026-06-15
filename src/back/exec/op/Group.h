#pragma once

#include "back/exec/Record.h"
#include "back/exec/op/Aggregate.h"
#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/Projection.h"

#include "core/types.h"
#include "util/containers.h"
#include "util/verify.h"

#include <vector>

namespace lsql::back::exec {

class GroupRecord : public Record {
 public:
    explicit GroupRecord(std::vector<Value> values) : values_(std::move(values)) {}

    const Value& value(SlotId slot) const override {
        verify_dbg(0 <= slot && slot < values_.size());
        return values_[slot];
    }

    ConstRecordPtr cloneImpl() const override { return std::make_shared<GroupRecord>(*this); }

 private:
    std::vector<Value> values_;
};

using AggregateProjectionMap = std::unordered_map<FieldId, AggregateProjector*>;

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
        , projectors_(std::move(aggregators))
        , projectors_map_(toProjectionMap(projectors_))
        , group_key_(std::move(group_key)) {
        prof::addEdge(sub_.scopeHandle(), prof_);

        for (auto&& proj : group_key_) {
            group_key_fields_.add(proj->field_id);
        }
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

            for (auto&& aggregator : aggregators) {
                if (aggregator != nullptr) {
                    aggregator->feed(*record);
                }
            }

            if (!active(phase)) {
                groups_.clear();
                return false;
            }

            return true;
        }

        // end of stream
        while (!groups_.empty()) {
            auto node = groups_.extract(groups_.begin());
            auto key = std::move(node.key());

            std::vector<Value> values;
            values.reserve(projectors_.size() + group_key_.size());

            // schema: <aggregators...>, <group key...>
            auto aggregators = std::move(node.mapped());
            for (auto&& aggregator : aggregators) {
                if (aggregator != nullptr) {
                    values.push_back(aggregator->get());
                } else {
                    values.push_back(vnull);
                }
            }

            util::append(values, std::move(key));

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
        for (auto&& proj : group_key_) {
            upstream.merge(proj->expr->requiredFields());
        }

        for (auto&& id : downstream.fieldIds()) {
            if (group_key_fields_.contains(id)) {
                // already requested
                continue;
            }

            auto it = projectors_map_.find(id);
            verify(it != projectors_map_.end());
            upstream.merge(it->second->expr->requiredFields());
        }

        return upstream;
    }

    void prepareAggregators(int phase, auto& aggregators) {
        aggregators.clear();
        aggregators.reserve(projectors_.size());

        auto&& required_fields = requiredFields(phase);

        for (auto&& projector : projectors_) {
            if (required_fields.contains(projector->field_id)) {
                aggregators.push_back(projector->expr->aggregator());
            } else {
                aggregators.push_back(nullptr);
            }
        }
    }

    std::vector<Value> getKey(const Record& record) {
        std::vector<Value> key;
        key.reserve(group_key_.size());
        for (auto&& proj : group_key_) {
            key.push_back(proj->expr->eval(record));
        }
        return key;
    }

    static AggregateProjectionMap toProjectionMap(AggregateProjectionList& list) {
        AggregateProjectionMap map;
        map.reserve(list.size());
        for (auto&& proj : list) {
            auto id = proj->field_id;
            map.emplace(id, proj.get());
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
    AggregateProjectionList projectors_;
    AggregateProjectionMap projectors_map_;
    std::vector<std::unique_ptr<ScalarProjector>> group_key_;
    FieldSet group_key_fields_;

    MemberSubscriber<Group> sub_{
        this,
        &Group::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    // phase state
    using Groups = std::unordered_map<std::vector<Value>, std::vector<AggregatorPtr>>;

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
