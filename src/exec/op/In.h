#pragma once

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
        if (record == nullptr) {
            // not emitting because it's not the last phase
            return false;
        }

        auto val = record->values();
        if (val.size() != 1) {
            throw std::runtime_error("expected exactly 1 column in IN rhs");
        }
        values_[phase].insert(val.begin()->second);

        return active(phase + 1);
    }

    bool consumeSource(int phase, const exec::Record* record) {
        if (record == nullptr) {
            values_.erase(phase - 1);
            return emit(phase, nullptr);
        }

        auto it = values_.find(phase - 1);
        if (it == values_.end()) {
            return emit(phase, nullptr);
        }

        if (it->second.contains(proj_->eval(*record))) {
            return emit(phase, record);
        }

        if (active(phase)) {
            return true;
        }

        values_.erase(it);
        return false;
    }

    void subscribe(int in_phase) override {
        match_source_->subscribe(in_phase, &sub_match_);
        source_->subscribe(in_phase + 1, &sub_source_);
    }

    OperationPtr source_;
    OperationPtr match_source_;
    ExpressionPtr proj_;
    MemberSubscriber<In> sub_source_{this, &In::consumeSource};
    MemberSubscriber<In> sub_match_{this, &In::consumeMatch};

    // phase -> matches
    std::unordered_map<int, std::unordered_set<Value>> values_;
};

OperationPtr in(OperationPtr source, OperationPtr match, ExpressionPtr proj) {
    return std::make_shared<In>(std::move(source), std::move(match), std::move(proj));
}

}  // namespace lsql::exec
