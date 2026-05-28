#pragma once

#include "util/Pinned.h"
#include "util/instrument/Histogram.h"
#include "util/instrument/duration.h"
#include "util/instrument/types.h"

#include <chrono>

namespace lsql::instr {

template <typename Duration = std::chrono::microseconds, size_t Bits = 31, size_t Resolution = 1>
class SequenceProfile {
    struct Item : util::Pinned {
        explicit Item(SequenceProfile* p) : profile_(p), started_at_(MonotonicClock::now()) {}

        void done() { std::exchange(profile_, nullptr)->push(MonotonicClock::now() - started_at_); }

        ~Item() {
            if (!profile_) {
                return;
            }
            done();
        }

     private:
        SequenceProfile* profile_;
        MonotonicTimePoint started_at_;
    };

 public:
    SequenceProfile() = default;

    bool empty() const { return count_ == 0; }

    Item scope() { return Item(this); }

    void reset() {
        count_ = 0;
        total_duration_ = {};
        hist_.reset();
    }

    void add(MonotonicDuration duration) {
        ++count_;
        total_duration_ += duration;

        Duration dur = std::chrono::duration_cast<Duration>(duration);
        hist_.add(dur.count());
    }

    std::string format() const {
        return std::format(
            "count={} total={} avg={} {}",
            count_,
            prettyDuration(total_duration_),
            prettyDuration(count_ > 0 ? total_duration_ / count_ : Duration{}),
            to_string<Duration>(hist_));
    }

 private:
    uint32_t count_ = 0;
    MonotonicDuration total_duration_ = {};
    util::Histogram<Bits, Resolution> hist_;
};

}  // namespace lsql::instr
