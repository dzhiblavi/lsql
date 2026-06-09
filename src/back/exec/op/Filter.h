#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/OperationBase.h"

namespace lsql::back::exec {

class Filter : public OperationBase<Filter> {
 public:
    Filter(OperationPtr source, ScalarPtr condition, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , condition_(std::move(condition)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

 private:
    // Subscriber
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        if (condition_->eval(*record).get<bool>()) {
            return emit(phase, record);
        }

        return active(phase);
    }

    // Operation
    void init(int out_phase, const FieldSet& downstream) override {
        FieldSet upstream = FieldSet::merge(condition_->requiredFields(), downstream);
        source_->subscribe(out_phase, &sub_, upstream);
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
    ScalarPtr condition_;

    MemberSubscriber<Filter> sub_{
        this,
        &Filter::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

OperationPtr filter(OperationPtr source, ScalarPtr condition, ConstFieldBindingPtr binding) {
    return std::make_shared<Filter>(std::move(source), std::move(condition), std::move(binding));
}

}  // namespace lsql::back::exec
