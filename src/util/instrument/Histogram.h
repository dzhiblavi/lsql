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
    inline static const int CLZLCorrection = __builtin_clzl(uint32_t(0)) - 32;

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
        auto lz = 31 - (__builtin_clzl(value) - CLZLCorrection);

        assert(lz / Resolution < BucketsCount);
        ++buckets[lz / Resolution];
    }

    void reset() {
        for (size_t i = 0; i < BucketsCount; ++i) {
            buckets[i] = 0;
        }
    }

    static constexpr unsigned long bucketEdge(uint32_t bucket) {
        return 1UL << ((1UL + bucket) * Resolution);
    }

    uint32_t buckets[BucketsCount];
};

template <size_t Bits = 31, size_t Resolution = 1>
auto histogram() {
    return Histogram<Bits, Resolution>();
}

}  // namespace lsql::util
