#pragma once

#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"
#include "back/exec/phys/Source.h"

namespace lsql::back::exec::phys {

struct MaterializeState {
    std::vector<ConstRecordPtr> records;
};

class MaterializeEmitter : public Source, public OperationBase<MaterializeEmitter> {
 public:
    MaterializeEmitter(int id, Arc<MaterializeState> state)
        : OperationBase(id)
        , state_(std::move(state)) {}

    void push() override {
        for (auto&& record : state_->records) {
            if (!emit(record.get())) {
                return;
            }
        }

        emit(nullptr);
    }

 protected:
    Arc<MaterializeState> state_;
};

class MaterializeCollector : public OperationBase<MaterializeCollector> {
 public:
    MaterializeCollector(int id, Arc<MaterializeState> state)
        : OperationBase(id)
        , state_(std::move(state)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    // Subscriber
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        state_->records.push_back(record->clone());
        return emit(record);
    }

    Arc<MaterializeState> state_;
    MemberSubscriber<MaterializeCollector> sub_{
        this,
        &MaterializeCollector::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

}  // namespace lsql::back::exec::phys
