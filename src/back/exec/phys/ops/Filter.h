#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class Filter : public OperationBase<Filter> {
 public:
    Filter(int id, Arc<Scalar> condition) : OperationBase(id), condition_(std::move(condition)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        if (condition_->eval(*record).get<bool>()) {
            return emit(record);
        }

        return active();
    }

    Arc<Scalar> condition_;
    MemberSubscriber<Filter> sub_{
        this,
        &Filter::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

}  // namespace lsql::back::exec::phys
