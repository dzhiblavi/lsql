#pragma once

#include "exec/expr/Scalar.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"

namespace lsql::exec {

class Filter : public OperationBase<Filter> {
 public:
    Filter(OperationPtr source, ScalarPtr condition, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , condition_(std::move(condition)) {}

 private:
    // Subscriber
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        if (trueish(condition_->eval(*record))) {
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
        prof_.inputHandle(&sub_),
    };
};

OperationPtr filter(OperationPtr source, ScalarPtr condition, ConstFieldBindingPtr binding) {
    return std::make_shared<Filter>(std::move(source), std::move(condition), std::move(binding));
}

}  // namespace lsql::exec
