#pragma once

#include "core/verify.h"
#include "exec/expr/Expression.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"

namespace lsql::exec {

class SemiJoin : public OperationBase<SemiJoin> {
 public:
    SemiJoin(
        OperationPtr source,
        OperationPtr match_source,
        ExpressionPtr proj,
        FieldId match_field_id,
        ConstFieldBindingPtr binding)
        : OperationBase(
              std::max(source->minPhase(), match_source->minPhase() + 1), std::move(binding))
        , source_(std::move(source))
        , match_source_(std::move(match_source))
        , proj_(std::move(proj))
        , match_field_id_(match_field_id) {}

 private:
    bool consumeMatch(int phase, const Record* record) {
        verify(phase == match_phase_);

        if (record == nullptr) {
            // not emitting because it's not the last phase
            return false;
        }

        values_.insert(record->value(match_field_id_));
        return active(phase + 1);
    }

    bool consumeSource(int phase, const Record* record) {
        if (record == nullptr) {
            cleanIfDone(phase);
            return emit(phase, nullptr);
        }

        if (values_.contains(proj_->eval(*record))) {
            return emit(phase, record);
        }

        if (active(phase)) {
            return true;
        }

        cleanIfDone(phase);
        return false;
    }

    void init(int out_phase, const FieldSet& downstream) override {
        verify(out_phase >= minPhase());

        if (match_phase_ == -1) {
            match_phase_ = out_phase - 1;
            match_source_->subscribe(
                out_phase - 1, &sub_match_, FieldSet::withField(match_field_id_));
        }

        // this may be an incorrect expectation
        verify(out_phase > match_phase_);

        source_->subscribe(
            out_phase, &sub_source_, FieldSet::merge(proj_->requiredFields(), downstream));
    }

    void cleanIfDone(int phase) {
        if (phase < maxPhase()) {
            return;
        }

        values_ = {};
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto match = match_source_->explain(ctx.withRequester(&sub_match_));
        auto source = source_->explain(ctx.withRequester(&sub_source_));

        if (ctx.phase < match_phase_) {
            // we do nothing on this phase
            verify(match.empty());
            verify(source.empty());
            return {};
        }

        if (ctx.phase == match_phase_) {
            verify(source.empty());

            auto item =
                ExplanationItem().line("{}: store match set", description(ctx.phase)).child(match);

            if (hasSubscriber(ctx.phase, ctx.requester)) {
                return item;
            } else {
                ctx.explanation.insert(item, this);
                return {};
            }
        }

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{}: stream match", description(ctx.phase)).child(source);
    }

    OperationPtr source_;
    OperationPtr match_source_;
    ExpressionPtr proj_;
    FieldId match_field_id_;

    MemberSubscriber<SemiJoin> sub_source_{
        this,
        &SemiJoin::consumeSource,
        prof_.inputHandle(&sub_source_),
    };
    MemberSubscriber<SemiJoin> sub_match_{
        this,
        &SemiJoin::consumeMatch,
        prof_.inputHandle(&sub_match_),
    };

    // phase at which values_ are built
    int match_phase_ = -1;
    std::unordered_set<Value> values_;
};

OperationPtr semiJoin(
    OperationPtr source,
    OperationPtr match,
    ExpressionPtr proj,
    FieldId match_field_id,
    ConstFieldBindingPtr binding) {
    return std::make_shared<SemiJoin>(
        std::move(source), std::move(match), std::move(proj), match_field_id, std::move(binding));
}

}  // namespace lsql::exec
