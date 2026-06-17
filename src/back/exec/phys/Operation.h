#pragma once

#include "back/exec/phys/Subscriber.h"
#include "util/string.h"

#include <absl/container/flat_hash_set.h>
#include <rfl/type_name_t.hpp>

namespace lsql::back::exec::phys {

class Operation {
 public:
    virtual ~Operation() = default;
    virtual void output(Subscriber* subscriber) = 0;
};

template <typename Self, typename... CustomScopeMetrics>
class OperationBase : public virtual Operation {
    using MetricsType = prof::ScopeMetrics<CustomScopeMetrics...>;

 public:
    explicit OperationBase(int id)
        : id_(id)
        , prof_(prof::newScope<MetricsType>("{} emitter", name())) {}

    void output(Subscriber* subscriber) override {
        verify(!subscribers_.contains(subscriber));
        subscribers_.insert(subscriber);
        prof::addEdge(prof_, subscriber->scopeHandle());
    }

 protected:
    bool active() const { return !subscribers_.empty(); }

    bool emit(const Record* record) {
        auto _ = prof_.scope();
        verify_dbg(!subscribers_.empty());

        auto it = subscribers_.begin();
        while (it != subscribers_.end()) {
            bool cont = (*it)->consume(record);

            if (cont) {
                ++it;
            } else {
                subscribers_.erase(it++);
            }
        }

        return active();
    }

    std::string name() const {
        auto class_name = util::stripNamespace(rfl::type_name_t<Self>().name());
        return std::format("{} [id={}]", class_name, id_);
    }

    const int id_;
    prof::ScopeHandle<MetricsType> prof_;

 private:
    absl::flat_hash_set<Subscriber*> subscribers_;
};

}  // namespace lsql::back::exec::phys
