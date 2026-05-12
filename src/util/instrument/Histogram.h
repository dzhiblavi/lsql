#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace lsql::util {

// value: 0 0 0 0 0 0 0 0 0 0
//                |  Bits   |   -- looking for this bits only
// BucketCount = Bits / Resolution
// Max accepted Bits value is 31
template <size_t Bits = 31, size_t Resolution = 1>
class Histogram {
    static_assert(Bits <= 31);
    static_assert(Bits % Resolution == 0);
    static constexpr size_t IgnoredLeadingBits = 32 - Bits;
    inline static const int CLZLOffset = __builtin_clzl(1);

 public:
    static constexpr size_t BucketsCount = Bits / Resolution;

    Histogram() = default;

    void add(uint32_t value) {
        // mask is 0 0 0 0  1 1 1 1
        //         | ILB | | Bits |
        static constexpr uint32_t mask = ~((uint32_t(1ULL << IgnoredLeadingBits) - 1) << Bits);

        // protect against zeros
        value = value | 1;

        // zero ignored bits
        value = value & mask;

        // get leading zero index
        auto lz = CLZLOffset - __builtin_clzl(value);

        assert(lz / Resolution < BucketsCount);
        ++buckets_[lz / Resolution];
    }

    void reset() {
        for (size_t i = 0; i < BucketsCount; ++i) {
            buckets_[i] = 0;
        }
    }

    uint32_t operator[](size_t bucket) const {
        assert(bucket < BucketsCount);
        return buckets_[bucket];
    }

    static constexpr unsigned long bucketMax(uint32_t bucket) {
        int max_lz = bucket * Resolution;
        int max_ones = max_lz + 1;
        return (1UL << max_ones) - 1;
    }

 private:
    uint32_t buckets_[BucketsCount] = {0};
};

template <size_t Bits = 31, size_t Resolution = 1>
auto histogram() {
    return Histogram<Bits, Resolution>();
}

}  // namespace lsql::util
