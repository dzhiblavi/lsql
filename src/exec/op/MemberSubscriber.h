#pragma once

#include "core/verify.h"
#include "exec/op/Profiler.h"
#include "exec/op/Subscriber.h"

namespace lsql::exec {

template <typename Self>
class MemberSubscriber : public Subscriber {
 public:
    using MethodType = bool (Self::*)(int, const Record*);

    MemberSubscriber(Self* self, MethodType method, prof::InputHandle handle)
        : self_(self)
        , method_(method)
        , prof_(handle) {
        verify(self != nullptr);
        verify(method != nullptr);
    }

    bool consume(int phase, const Record* record) override {
        auto _ = prof_.consumeScope();
        return (self_->*method_)(phase, record);
    }

 private:
    Self* self_;
    MethodType method_;
    prof::InputHandle prof_;
};

}  // namespace lsql::exec
