#pragma once

#include "core/verify.h"
#include "exec/op/Subscriber.h"
#include "exec/prof/InputHandle.h"

#include <tuple>

namespace lsql::exec {

template <typename F>
concept SubscriberMixin = requires(F& f) {
    typename F::ScopeValueType;
    { f.beforeConsume() } -> std::same_as<typename F::ScopeValueType>;
};

template <typename Self, SubscriberMixin... Mixins>
class MemberSubscriberBase : public Subscriber {
 public:
    using MethodType = bool (Self::*)(int, const Record*);

    template <typename... Args>
    MemberSubscriberBase(Self* self, MethodType method, Args&&... args)
        : self_(self)
        , method_(method)
        , mixins_(std::forward<Args>(args)...) {
        verify(self != nullptr);
        verify(method != nullptr);
    }

    bool consume(int phase, const Record* record) override {
        auto scope = beforeConsume();
        return (self_->*method_)(phase, record);
    }

 private:
    std::tuple<typename Mixins::ScopeValueType...> beforeConsume() {
        return std::apply(
            [](Mixins&... mixins) { return std::tuple{mixins.beforeConsume()...}; }, mixins_);
    }

    Self* self_;
    MethodType method_;
    std::tuple<Mixins...> mixins_;
};

struct ProfilerMixin {
    using ScopeValueType = decltype(prof::InputHandle().consumeScope());
    explicit ProfilerMixin(prof::InputHandle handle) : handle_(std::move(handle)) {}
    ScopeValueType beforeConsume() { return handle_.consumeScope(); }

 private:
    prof::InputHandle handle_;
};

struct LockMixin {
    using ScopeValueType = std::unique_lock<std::mutex>;
    explicit LockMixin(std::mutex* m) : m_(m) {}
    ScopeValueType beforeConsume() { return std::unique_lock(*m_); }

 private:
    std::mutex* m_;
};

// ProfilerMixin should be last because Mixins... can optionally
// control concurrency (ProfilerMixin is not threadsafe).
template <typename Op, SubscriberMixin... Mixins>
using MemberSubscriber = MemberSubscriberBase<Op, Mixins..., ProfilerMixin>;

}  // namespace lsql::exec
