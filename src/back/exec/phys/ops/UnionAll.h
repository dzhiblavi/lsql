#pragma once

#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class UnionAll : public OperationBase<UnionAll> {
 public:
    UnionAll(size_t num_inputs, int id) : OperationBase(id) {
        for (size_t i = 0; i < num_inputs; ++i) {
            subs_.emplace_back(
                this,
                &UnionAll::consume,
                prof::newScope<ScopeMetrics>("{} input({})", name(), i),
                &m_);

            prof::addEdge(subs_.back().scopeHandle(), prof_);
        }
    }

    Subscriber* sub(size_t index) { return &subs_[index]; }

 private:
    bool consume(const Record* record) {
        if (stream_finished_) {
            return false;
        }

        if (record == nullptr) {
            ++eof_count_;
            if (eof_count_ == subs_.size()) {
                stream_finished_ = true;
                emit(nullptr);
            }
            return false;
        }

        if (!emit(record)) {
            stream_finished_ = true;
            return false;
        }

        return true;
    }

    bool stream_finished_ = false;
    size_t eof_count_ = 0;

    std::mutex m_;
    std::vector<MemberSubscriber<UnionAll, LockMixin>> subs_;
};

}  // namespace lsql::back::exec::phys
