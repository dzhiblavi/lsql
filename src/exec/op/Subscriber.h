#pragma once

#include "core/verify.h"
#include "exec/Record.h"
#include "exec/op/Profiler.h"

namespace lsql::exec {

class Subscriber {
 public:
    virtual ~Subscriber() = default;

    // record == nullptr means that the stream has ended
    // result means "continue feeding me records"
    // not requried to be threadsafe, thus must not be called concurrently
    virtual bool consume(int phase, const exec::Record* record) = 0;
};

template <typename Self>
class MemberSubscriber : public Subscriber {
 public:
    using MethodType = bool (Self::*)(int, const exec::Record*);

    MemberSubscriber(Self* self, MethodType method, InputHandle handle)
        : self_(self)
        , method_(method)
        , handle_(handle) {
        verify(self != nullptr);
        verify(method != nullptr);
    }

    bool consume(int phase, const exec::Record* record) override {
        auto _ = handle_.consumeScope();
        return (self_->*method_)(phase, record);
    }

 private:
    Self* self_;
    MethodType method_;
    InputHandle handle_;
};

}  // namespace lsql::exec
