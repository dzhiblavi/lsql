#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/OperationBase.h"

#include <absl/container/flat_hash_map.h>

#include <unordered_map>
#include <vector>

namespace lsql::back::exec {

struct ScalarProjector {
    FieldId field_id;
    ScalarPtr expr;
};

using ScalarProjectorPtr = std::unique_ptr<ScalarProjector>;
using ScalarProjectionList = std::vector<std::unique_ptr<ScalarProjector>>;
using ScalarProjectionMap = std::unordered_map<FieldId, ScalarProjector*>;

class ScalarProjectionRecord : public Record {
 public:
    explicit ScalarProjectionRecord(std::vector<Value> values) : values_(std::move(values)) {}

    const Value& value(SlotId slot) const override {
        verify_dbg(
            0 <= slot && slot < values_.size(),
            "slot {} out of range {}",
            uint32_t(slot),
            values_.size());

        return values_[slot];
    }

    ConstRecordPtr cloneImpl() const override {
        return std::make_shared<ScalarProjectionRecord>(values_);
    }

 private:
    std::vector<Value> values_;
};

class Projection : public OperationBase<Projection>,
                   public std::enable_shared_from_this<Projection> {
 public:
    Projection(OperationPtr source, ScalarProjectionList projectors, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , projectors_(std::move(projectors))
        , projectors_map_(buildProjectionMap(projectors_)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

 private:
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        auto&& phase_projectors = phase_projectors_[phase];
        std::vector<Value> values;
        values.reserve(projectors_.size());
        for (auto* scalar : phase_projectors) {
            if (scalar != nullptr) {
                values.push_back(scalar->expr->eval(*record));
            } else {
                values.push_back(vnull);
            }
        }

        ScalarProjectionRecord rec(std::move(values));
        return emit(phase, &rec);
    }

    void init(int phase, const FieldSet& downstream) override {
        source_->subscribe(phase, &sub_, getFieldSet(downstream));
        precalcProjectors(phase);
    }

    void precalcProjectors(int phase) {
        auto&& required = requiredFields(phase);
        auto& phase_projectors = phase_projectors_[phase];

        phase_projectors.clear();
        for (auto&& projector : projectors_) {
            if (required.contains(projector->field_id)) {
                phase_projectors.push_back(projector.get());
            } else {
                phase_projectors.push_back(nullptr);
            }
        }
    }

    FieldSet getFieldSet(const FieldSet& downstream) const {
        FieldSet result = FieldSet::emptySet();

        for (auto&& id : downstream.fieldIds()) {
            if (auto it = projectors_map_.find(id); it != projectors_map_.end()) {
                result.merge(it->second->expr->requiredFields());
            }
        }

        return result;
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{}", description(ctx.phase)).child(source);
    }

    ScalarProjectionMap buildProjectionMap(ScalarProjectionList& proj) {
        ScalarProjectionMap res;
        res.reserve(proj.size());

        for (auto&& p : proj) {
            auto id = p->field_id;
            res.emplace(id, p.get());
        }

        return res;
    }

    OperationPtr source_;
    std::vector<std::unique_ptr<ScalarProjector>> projectors_;
    ScalarProjectionMap projectors_map_;
    absl::flat_hash_map<int, std::vector<ScalarProjector*>> phase_projectors_;

    MemberSubscriber<Projection> sub_{
        this,
        &Projection::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

OperationPtr projection(
    OperationPtr source, ScalarProjectionList slist, ConstFieldBindingPtr binding) {
    return std::make_shared<Projection>(std::move(source), std::move(slist), std::move(binding));
}

}  // namespace lsql::back::exec
