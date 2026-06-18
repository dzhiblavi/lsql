#pragma once

#include <mutex>
#include <ostream>
#include <sstream>
#include <string_view>

namespace lsql::util {

class OrderedSink {
    enum State : uint8_t {
        ConsumingWaiting,
        ConsumingReady,
        DoneWaiting,
        DoneComplete,
    };

 public:
    OrderedSink(std::ostream* os, std::mutex* lock, bool should_wait);

    void setNext(OrderedSink* next);
    void push(std::string_view s);
    void done();

 private:
    void flushLocked();
    void notifyNextLocked();
    void prevCompleteLocked();

    std::ostream* os_;
    std::mutex* m_;
    OrderedSink* next_ = nullptr;

    std::stringstream buf_;
    State state_;
};

}  // namespace lsql::util
