#pragma once

#include "core/verify.h"
#include "exec/Record.h"
#include "exec/op/Projection.h"

#include <vector>

namespace lsql::exec {

using GroupValues = std::unordered_map<std::string_view, Value>;

class GroupEnrichedRecord : public exec::Record {
 public:
    GroupEnrichedRecord(RecordRef child, std::shared_ptr<GroupValues> values)
        : child_(std::move(child))
        , values_(std::move(values)) {}

    values_t values() const override {
        auto values = get(child_)->values();
        for (auto&& [k, v] : *values_) {
            values.emplace(k, v);
        }
        return values;
    }

    Value value(std::string_view name) const override {
        if (auto it = values_->find(name); it != values_->end()) {
            return it->second;
        }
        return get(child_)->value(name);
    }

    std::shared_ptr<const Record> clone() const override {
        return std::make_shared<GroupEnrichedRecord>(pin(child_), values_);
    }

 private:
    RecordRef child_;
    std::shared_ptr<GroupValues> values_;
};

class GroupRecord : public exec::Record {
 public:
    explicit GroupRecord(std::shared_ptr<GroupValues> values) : values_(std::move(values)) {}

    values_t values() const override {
        values_t values;
        for (auto&& [k, v] : *values_) {
            values.emplace(k, v);
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = values_->find(name);
        return it == values_->end() ? null : it->second;
    }

    exec::ConstRecordPtr clone() const override { return std::make_shared<GroupRecord>(*this); }

 private:
    std::shared_ptr<GroupValues> values_;
};

class Group : public Operation, public std::enable_shared_from_this<Group> {
    friend class GroupRecord;

 public:
    Group(OperationPtr source, ProjectionList glist, ProjectionList slist)
        : Operation(source->minPhase())
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
            verify(groups_.empty());
        }

        if (record != nullptr) {
            auto group_kv = std::make_shared<GroupValues>();
            std::vector<Value> key;
            key.reserve(glist_.size());
            for (auto&& col : glist_) {
                auto value = col->expr->eval(*record);
                group_kv->emplace(col->name, value);
                key.push_back(std::move(value));
            }

            auto&& aggregators = groups_[key];

            if (aggregators.empty()) {
                for (auto&& proj : slist_) {
                    auto&& name = proj->name;

                    if (std::ranges::find(glist_, name, [](auto&& proj) { return proj->name; }) ==
                        glist_.end()) {
                        aggregators.push_back(proj->expr->aggregator());
                    }
                }
            }

            for (auto&& aggregator : aggregators) {
                GroupEnrichedRecord enriched_record(record, std::move(group_kv));
                aggregator->feed(enriched_record);
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
            auto group_values = std::move(node.key());
            auto aggregators = std::move(node.mapped());

            GroupValues group_kv;
            for (size_t i = 0; i < glist_.size(); ++i) {
                group_kv.emplace(glist_[i]->name, std::move(group_values[i]));
            }

            auto values = std::make_shared<GroupValues>();
            size_t agr = 0;

            for (size_t i = 0; i < slist_.size(); ++i) {
                auto&& name = slist_[i]->name;
                auto it = std::ranges::find(glist_, name, [](auto&& proj) { return proj->name; });

                if (it == glist_.end()) {
                    values->emplace(name, aggregators[agr++]->get());
                } else {
                    values->emplace(name, group_kv[name]);
                }
            }

            GroupRecord record(std::move(values));
            if (!emit(phase, &record)) {
                groups_.clear();
                return false;
            }
        }

        return emit(phase, nullptr);
    }

    // Operation
    void init(int phase) override { source_->subscribe(phase, &sub_); }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("Group").child(source);
    }

    OperationPtr source_;
    ProjectionList glist_;
    ProjectionList slist_;
    MemberSubscriber<Group> sub_{this, &Group::consume};

    // phase state
    using Groups = std::unordered_map<std::vector<Value>, std::vector<exec::AggregatorPtr>>;

    int curr_phase_ = 0;
    Groups groups_;
};

OperationPtr group(OperationPtr source, ProjectionList glist, ProjectionList slist) {
    return std::make_shared<Group>(std::move(source), std::move(glist), std::move(slist));
}

}  // namespace lsql::exec
