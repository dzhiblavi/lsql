#pragma once

#include "exec/Record.h"

namespace lsql::exec {

class Subscriber {
 public:
    virtual ~Subscriber() = default;

    // record == nullptr means that the stream has ended
    // result means "continue feeding me records"
    virtual bool consume(int phase, const exec::Record* record) = 0;
};

template <typename Self>
class MemberSubscriber : public Subscriber {
 public:
    using MethodType = bool (Self::*)(int, const exec::Record*);

    MemberSubscriber(const MemberSubscriber&) = default;
    MemberSubscriber& operator=(const MemberSubscriber&) = default;

    MemberSubscriber(Self* self, MethodType method) : self_(self), method_(method) {
        assert(self != nullptr);
        assert(method != nullptr);
    }

    bool consume(int phase, const exec::Record* record) override {
        return (self_->*method_)(phase, record);
    }

 private:
    Self* self_;
    MethodType method_;
};

}  // namespace lsql::exec
