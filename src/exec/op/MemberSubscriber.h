#pragma once

#include "exec/op/ScopeMetrics.h"
#include "exec/op/Subscriber.h"

#include "prof/Scope.h"

#include "core/verify.h"

#include <tuple>

namespace lsql::exec {

template <typename F>
concept SubscriberMixin = requires(F& f) {
    typename F::ScopeValueType;
    { f.consumeScope() } -> std::same_as<typename F::ScopeValueType>;
};

template <typename Self, SubscriberMixin... Mixins>
class MemberSubscriber : public Subscriber {
 public:
    using MethodType = bool (Self::*)(int, const Record*);

    template <typename... Args>
    MemberSubscriber(
        Self* self, MethodType method, prof::ScopeHandle<ScopeMetrics<>>* handle, Args&&... args)
        : self_(self)
        , method_(method)
        , mixins_(std::forward<Args>(args)...)
        , handle_(handle) {
        verify(self != nullptr);
        verify(method != nullptr);
    }

    bool consume(int phase, const Record* record) override {
        [[maybe_unused]] auto scope = consumeScope();
        auto _ = handle_->scope();
        return (self_->*method_)(phase, record);
    }

    prof::MetricsBase* profHandle() override { return handle_->metrics(); }

 private:
    std::tuple<typename Mixins::ScopeValueType...> consumeScope() {
        return std::apply(
            [](Mixins&... mixins) { return std::tuple{mixins.consumeScope()...}; }, mixins_);
    }

    Self* self_;
    MethodType method_;
    std::tuple<Mixins...> mixins_;
    prof::ScopeHandle<ScopeMetrics<>>* handle_;
};

struct LockMixin {
    using ScopeValueType = std::unique_lock<std::mutex>;
    explicit LockMixin(std::mutex* m) : m_(m) {}
    ScopeValueType consumeScope() { return std::unique_lock(*m_); }

 private:
    std::mutex* m_;
};

}  // namespace lsql::exec
