#pragma once

#include "util/Pinned.h"
#include "util/instrument/Histogram.h"
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

    Item next() { return Item(this); }
    void reset() { hist_.reset(); }

    std::string report() {
        std::stringstream ss;

        ss << '[';

        for (size_t bucket = 0; bucket < hist_.BucketsCount; ++bucket) {
            if (hist_.buckets[bucket] == 0) {
                continue;
            }

            ss << std::format(
                "<={}: {}, ", Duration(hist_.bucketEdge(bucket)), hist_.buckets[bucket]);
        }

        ss << ']';
        return std::move(ss).str();
    }

 private:
    void push(MonotonicDuration duration) {
        Duration dur = std::chrono::duration_cast<Duration>(duration);
        hist_.add(dur.count());
    }

    util::Histogram<Bits, Resolution> hist_;
};

}  // namespace lsql::instr
