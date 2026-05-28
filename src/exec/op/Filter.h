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
        , condition_(std::move(condition)) {
        prof::addEdge(&prof_sub_, &prof_);
    }

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

    static bool trueish(const Value& val) {
        return val.visit(
            util::Overloaded{
                [](const std::string& s) { return !s.empty(); },
                [](bool b) { return b; },
                [](int64_t x) { return x != 0; },
                [](float x) { return abs(x) > 1e-6f; },
                [](null_t) { return false; },
            });
    }

    OperationPtr source_;
    ScalarPtr condition_;

    prof::ScopeHandle<ScopeMetrics> prof_sub_ = prof::newScope<ScopeMetrics>("{} input", name());

    MemberSubscriber<Filter> sub_{
        this,
        &Filter::consume,
        &prof_sub_,
    };
};

OperationPtr filter(OperationPtr source, ScalarPtr condition, ConstFieldBindingPtr binding) {
    return std::make_shared<Filter>(std::move(source), std::move(condition), std::move(binding));
}

}  // namespace lsql::exec
