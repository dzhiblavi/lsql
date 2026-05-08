#pragma once

#include "exec/op/Subscriber.h"

#include <memory>
#include <unordered_set>

namespace lsql::exec {

class Operation {
 public:
    Operation(int phases, int min_out_phase) : phases_(phases), min_out_phase_(min_out_phase) {}

    virtual ~Operation() = default;

    // outbound operations call this method to receive records
    void subscribe(int out_phase, Subscriber* sub) {
        assert(out_phase >= minPhase());

        if (subs_[out_phase].contains(sub)) {
            return;
        }

        max_out_phase_ = std::max(max_out_phase_, out_phase);
        subs_[out_phase].insert(sub);

        int in_phase = out_phase - phases_ + 1;
        subscribe(in_phase);
    }

    int minPhase() const { return min_out_phase_; }
    int maxPhase() const { return max_out_phase_; }

 protected:
    // must subscribe for phases starting from `in_phase` in inbound operations
    virtual void subscribe(int in_phase) = 0;

    bool active(int phase) const {
        auto it = subs_.find(phase);
        return it != subs_.end() && !it->second.empty();
    }

    // returns active(phase)
    bool emit(int phase, const exec::Record* record) {
        assert(active(phase));

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

    // total phases in this operation
    const int phases_ = 0;

    // the phase the result is available at
    const int min_out_phase_ = 0;

 protected:
    // the max out phase
    int max_out_phase_ = 0;

    // global phase -> subscribers
    std::unordered_map<int, std::unordered_set<Subscriber*>> subs_;
};

using OperationPtr = std::shared_ptr<Operation>;

}  // namespace lsql::exec
