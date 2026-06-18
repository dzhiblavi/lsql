#include "util/OrderedSink.h"

#include "util/verify.h"

namespace lsql::util {

OrderedSink::OrderedSink(std::ostream* os, std::mutex* lock, bool should_wait)
    : os_(os)
    , m_(lock)
    , state_(should_wait ? ConsumingWaiting : ConsumingReady) {
}

void OrderedSink::setNext(OrderedSink* next) {
    next_ = next;
}

void OrderedSink::push(std::string_view s) {
    buf_ << s << '\n';
}

void OrderedSink::done() {
    std::lock_guard lg(*m_);

    switch (state_) {
        case ConsumingWaiting:
            state_ = DoneWaiting;
            break;

        case ConsumingReady:
            state_ = DoneComplete;
            flushLocked();
            notifyNextLocked();
            break;

        default:
            panic("invalid state");
    }
}

void OrderedSink::flushLocked() {
    *os_ << buf_.str() << '\n';
    buf_ = {};
}

void OrderedSink::notifyNextLocked() {
    if (next_ != nullptr) {
        next_->prevCompleteLocked();
    }
}

void OrderedSink::prevCompleteLocked() {
    switch (state_) {
        case ConsumingWaiting:
            state_ = ConsumingReady;
            return;

        case DoneWaiting:
            state_ = DoneComplete;
            flushLocked();
            notifyNextLocked();
            return;

        case ConsumingReady:
        case DoneComplete:
            return;
    }
}

}  // namespace lsql::util
