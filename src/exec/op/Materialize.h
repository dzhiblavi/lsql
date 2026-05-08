#pragma once

#include "exec/op/Source.h"

namespace lsql::exec {

class Materialize : public Source {
 public:
    Materialize(OperationPtr source) : Source(1, source->minPhase()), source_(std::move(source)) {}

    void push(int phase) override {
        if (first_phase_ == -1) {
            return;
        }

        if (phase <= first_phase_) {
            // using consume() for the first phase
            return;
        }

        pushMaterialized(phase);
    }

 private:
    bool consume(int phase, const exec::Record* record) {
        assert(phase == first_phase_);

        if (!materialized_) {
            materialized_.emplace();
        }

        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        materialized_->push_back(record->clone());
        return emit(phase, record);
    }

    void pushMaterialized(int phase) {
        assert(materialized_.has_value());

        if (!active(phase)) {
            return;
        }

        for (auto&& record : *materialized_) {
            if (!emit(phase, record.get())) {
                return;
            }
        }

        emit(phase, nullptr);
    }

    void subscribe(int in_phase) override {
        if (first_phase_ != -1) {
            return;
        }

        first_phase_ = in_phase;
        source_->subscribe(first_phase_, &sub_);
    }

    OperationPtr source_;
    MemberSubscriber<Materialize> sub_{this, &Materialize::consume};
    std::optional<std::vector<exec::ConstRecordPtr>> materialized_ = std::nullopt;
    int first_phase_ = -1;
};

SourcePtr materialize(OperationPtr source) {
    return std::make_shared<Materialize>(std::move(source));
}

}  // namespace lsql::exec
