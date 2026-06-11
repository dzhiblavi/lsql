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
using ScalarProjectionMap = std::unordered_map<FieldId, std::unique_ptr<ScalarProjector>>;

class ScalarProjectionRecord : public Record {
 public:
    explicit ScalarProjectionRecord(absl::flat_hash_map<FieldId, Value> values)
        : values_(std::move(values)) {}

    Value value(FieldId id) const override {
        if (auto it = values_.find(id); it != values_.end()) {
            return it->second;
        }
        return null;
    }

    ConstRecordPtr cloneImpl() const override {
        return std::make_shared<ScalarProjectionRecord>(values_);
    }

 private:
    absl::flat_hash_map<FieldId, Value> values_;
};

class Projection : public OperationBase<Projection>,
                   public std::enable_shared_from_this<Projection> {
 public:
    Projection(OperationPtr source, ScalarProjectionList projectors, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , projectors_(buildProjectionMap(std::move(projectors))) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

 private:
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        auto&& phase_projectors = phase_projectors_[phase];
        absl::flat_hash_map<FieldId, Value> values;
        values.reserve(phase_projectors.size());
        for (auto&& scalar : phase_projectors) {
            values.emplace(scalar->field_id, scalar->expr->eval(*record));
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

        for (FieldId id : required.fieldIds()) {
            if (std::ranges::find(phase_projectors, id, [](auto&& s) { return s->field_id; }) !=
                phase_projectors.end()) {
                // already projected in this phase
                continue;
            }

            auto it = projectors_.find(id);
            verify(it != projectors_.end());

            auto&& scalar = it->second;
            phase_projectors.push_back(scalar.get());
        }
    }

    FieldSet getFieldSet(const FieldSet& downstream) const {
        FieldSet result = FieldSet::emptySet();

        for (auto&& id : downstream.fieldIds()) {
            if (auto it = projectors_.find(id); it != projectors_.end()) {
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

    ScalarProjectionMap buildProjectionMap(ScalarProjectionList proj) {
        ScalarProjectionMap res;
        res.reserve(proj.size());

        for (auto&& p : proj) {
            auto id = p->field_id;
            res.emplace(id, std::move(p));
        }

        return res;
    }

    OperationPtr source_;
    ScalarProjectionMap projectors_;
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
