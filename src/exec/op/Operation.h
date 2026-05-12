#pragma once

#include "core/verify.h"
#include "exec/op/Explanation.h"
#include "exec/op/Profiler.h"
#include "exec/op/Subscriber.h"
#include "util/uniq_id.h"

#include <memory>
#include <unordered_set>

namespace lsql::exec {

class Operation {
    using Subscribers = std::unordered_map<int, std::unordered_set<Subscriber*>>;

 public:
    Operation(int min_out_phase, OperationHandle handle)
        : min_out_phase_(min_out_phase)
        , handle_(handle) {}

    virtual ~Operation() = default;

    // outbound operations call this method to receive records
    // idempotent
    void subscribe(int out_phase, Subscriber* sub) {
        verify(out_phase >= minPhase());

        if (subs_[out_phase].contains(sub)) {
            return;
        }

        max_out_phase_ = std::max(max_out_phase_, out_phase);
        subs_[out_phase].insert(sub);

        init(out_phase);
    }

    virtual ExplanationItem explain(ExplanationCtx ctx) const = 0;

    int minPhase() const { return min_out_phase_; }
    int maxPhase() const { return max_out_phase_; }
    int uniqId() const { return uniq_id_; }
    const Subscribers& subscribers() const { return subs_; }

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
    bool emit(int phase, const exec::Record* record) {
        verify(active(phase));
        auto _ = handle_.emitScope();

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

 protected:
    // the max out phase
    int max_out_phase_ = 0;

    // global phase -> subscribers
    Subscribers subs_;

    // profiler handle
    OperationHandle handle_;
};

using OperationPtr = std::shared_ptr<Operation>;

}  // namespace lsql::exec
