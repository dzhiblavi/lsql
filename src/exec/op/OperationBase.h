#pragma once

#include "core/verify.h"
#include "exec/prof/Profiler.h"
#include "util/uniq_id.h"

#include "exec/op/Operation.h"

#include <unordered_set>

namespace lsql::exec {

template <typename Self>
class OperationBase : public virtual Operation {
    using Subscribers = std::unordered_map<int, std::unordered_set<Subscriber*>>;

 public:
    OperationBase(int min_out_phase, std::string_view name)
        : min_out_phase_(min_out_phase)
        , name_(name)
        , prof_(prof::Profiler::registerOperation(this)) {}

    // outbound operations call this method to receive records
    // idempotent
    void subscribe(int out_phase, Subscriber* sub) override {
        verify(out_phase >= minPhase());

        if (subs_[out_phase].contains(sub)) {
            return;
        }

        max_out_phase_ = std::max(max_out_phase_, out_phase);
        subs_[out_phase].insert(sub);

        init(out_phase);
    }

    int minPhase() const override { return min_out_phase_; }
    int maxPhase() const override { return max_out_phase_; }
    std::string name() const override { return std::format("{} [id={}]", name_, uniq_id_); }

 protected:
    // the way for downstream operations to ask for output on phase `out_phase`
    // idempotent
    virtual void init(int out_phase) = 0;

    bool hasSubscriber(int phase, const Subscriber* sub) const {
        auto it = subs_.find(phase);
        return it != subs_.end() && it->second.contains(const_cast<Subscriber*>(sub));  // NOLINT
    }

    bool active(int phase) const {
        auto it = subs_.find(phase);
        return it != subs_.end() && !it->second.empty();
    }

    // returns active(phase)
    bool emit(int phase, const Record* record) {
        verify(active(phase));
        auto _ = prof_.emitScope();

        auto&& subs = subs_[phase];
        auto it = subs.begin();

        while (it != subs.end()) {
            bool cont = (*it)->consume(phase, record);

            if (cont) {
                ++it;
            } else {
                it = subs.erase(it);
            }
        }

        return active(phase);
    }

    // the phase the result is available at
    const int min_out_phase_ = 0;
    const int uniq_id_ = util::uniqId();
    const std::string_view name_;

 protected:
    // the max out phase
    int max_out_phase_ = 0;

    // global phase -> subscribers
    Subscribers subs_;

    // profiler handle
    prof::OperationHandle prof_;
};

}  // namespace lsql::exec
