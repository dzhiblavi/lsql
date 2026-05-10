#pragma once

#include "core/verify.h"
#include "exec/expr/Expression.h"
#include "exec/op/Operation.h"

namespace lsql::exec {

class In : public Operation {
 public:
    In(OperationPtr source, OperationPtr match_source, ExpressionPtr proj)
        : Operation(2, std::max(source->minPhase(), match_source->minPhase() + 1))
        , source_(std::move(source))
        , match_source_(std::move(match_source))
        , proj_(std::move(proj)) {}

 private:
    bool consumeMatch(int phase, const exec::Record* record) {
        verify(phase == match_phase_);

        if (record == nullptr) {
            // not emitting because it's not the last phase
            return false;
        }

        auto val = record->values();
        if (val.size() != 1) {
            throw std::runtime_error("expected exactly 1 column in IN rhs");
        }

        values_.insert(val.begin()->second);
        return active(phase + 1);
    }

    bool consumeSource(int phase, const exec::Record* record) {
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

    void init(int out_phase) override {
        verify(out_phase >= minPhase());

        if (match_phase_ == -1) {
            match_phase_ = out_phase - 1;
            match_source_->subscribe(out_phase - 1, &sub_match_);
        }

        // this may be an incorrect expectation
        verify(out_phase > match_phase_);

        source_->subscribe(out_phase, &sub_source_);
    }

    void cleanIfDone(int phase) {
        if (phase < maxPhase()) {
            return;
        }

        values_ = {};
    }

    OperationPtr source_;
    OperationPtr match_source_;
    ExpressionPtr proj_;
    MemberSubscriber<In> sub_source_{this, &In::consumeSource};
    MemberSubscriber<In> sub_match_{this, &In::consumeMatch};

    // phase at which values_ are built
    int match_phase_ = -1;
    std::unordered_set<Value> values_;
};

OperationPtr in(OperationPtr source, OperationPtr match, ExpressionPtr proj) {
    return std::make_shared<In>(std::move(source), std::move(match), std::move(proj));
}

}  // namespace lsql::exec
