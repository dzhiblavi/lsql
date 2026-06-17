#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class Projection : public OperationBase<Projection> {
 public:
    Projection(int id, std::vector<Arc<Scalar>> scalars)
        : OperationBase(id)
        , scalars_(std::move(scalars)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        std::vector<Value> values;
        values.reserve(scalars_.size());
        for (auto&& scalar : scalars_) {
            if (scalar != nullptr) {
                values.push_back(scalar->eval(*record));
            } else {
                values.emplace_back(null);
            }
        }

        VecRecord rec(std::move(values));
        return emit(&rec);
    }

    std::vector<Arc<Scalar>> scalars_;
    MemberSubscriber<Projection> sub_{
        this,
        &Projection::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

}  // namespace lsql::back::exec::phys
