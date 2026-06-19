#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class Projection : public OperationBase<Projection> {
 public:
    Projection(int id, std::vector<Arc<Scalar>> scalars)
        : OperationBase(id)
        , scalars_(std::move(scalars))
        , record_(std::vector<Value>(scalars_.size(), vnull)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        auto&& values = record_.mutableValues();
        for (size_t i = 0; i < scalars_.size(); ++i) {
            auto&& scalar = scalars_[i];
            auto&& value = values[i];

            if (scalar != nullptr) {
                value = scalar->eval(*record);
            }
        }

        return emit(&record_);
    }

    std::vector<Arc<Scalar>> scalars_;
    VecRecord record_;

    MemberSubscriber<Projection> sub_{
        this,
        &Projection::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

}  // namespace lsql::back::exec::phys
