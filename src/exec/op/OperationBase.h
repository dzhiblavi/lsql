#pragma once

#include "core/verify.h"
#include "exec/prof/Profiler.h"
#include "util/uniq_id.h"

#include "exec/op/Operation.h"

#include <rfl.hpp>
#include <unordered_set>

namespace lsql::exec {

template <typename Self>
class OperationBase : public virtual Operation {
 public:
    explicit OperationBase(int min_out_phase)
        : prof_(prof::Profiler::registerOperation(this))
        , min_out_phase_(min_out_phase) {}

    // outbound operations call this method to receive records
    // idempotent
    void subscribe(int out_phase, Subscriber* sub, const RequiredFields& fields) override {
        verify(out_phase >= minPhase());

        // all of the following operations are idempotent
        updateRequiredFields(out_phase, fields);
        max_out_phase_ = std::max(max_out_phase_, out_phase);
        subs_[out_phase].insert(sub);
        init(out_phase, requiredFields(out_phase));
    }

    int minPhase() const override { return min_out_phase_; }
    int maxPhase() const override { return max_out_phase_; }

    std::string name() const override {
        auto class_name = rfl::type_name_t<Self>().name().substr(sizeof("lsql::exec::") - 1);
        return std::format("{} [id={}]", class_name, uniq_id_);
    }

 protected:
    // the way for downstream operations to ask for output on phase `out_phase`
    // idempotent
    virtual void init(int out_phase, const RequiredFields& fields) = 0;

    std::string description(int phase) const {
        return std::format("{} [required-out: {}]", name(), to_string(requiredFields(phase)));
    }

    void updateRequiredFields(int phase, const RequiredFields& fields) {
        auto& required_fields =
            phase_required_fields_.try_emplace(phase, RequiredFields::withNone()).first->second;
        required_fields.merge(fields);
    }

    const RequiredFields& requiredFields(int phase) const {
        static RequiredFields none = RequiredFields::withNone();

        if (auto it = phase_required_fields_.find(phase); it != phase_required_fields_.end()) {
            return it->second;
        }
        return none;
    }

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

 protected:
    prof::OperationHandle prof_;

 private:
    using Subscribers = std::unordered_map<int, std::unordered_set<Subscriber*>>;
    using PhaseRequiredFields = std::unordered_map<int, RequiredFields>;

    const int min_out_phase_ = 0;
    const int uniq_id_ = util::uniqId();
    int max_out_phase_ = 0;
    Subscribers subs_;
    PhaseRequiredFields phase_required_fields_;
};

}  // namespace lsql::exec
