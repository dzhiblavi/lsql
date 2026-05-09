#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/Operation.h"

namespace lsql::exec {

class Filter : public Operation {
 public:
    Filter(OperationPtr source, ExpressionPtr condition)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , condition_(std::move(condition)) {}

 private:
    bool consume(int phase, const exec::Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        if (trueish(condition_->eval(*record))) {
            return emit(phase, record);
        }

        return active(phase);
    }

    void subscribe(int out_phase) override { source_->subscribe(out_phase, &sub_); }

    OperationPtr source_;
    ExpressionPtr condition_;
    MemberSubscriber<Filter> sub_{this, &Filter::consume};
};

OperationPtr filter(OperationPtr source, ExpressionPtr condition) {
    return std::make_shared<Filter>(std::move(source), std::move(condition));
}

}  // namespace lsql::exec
