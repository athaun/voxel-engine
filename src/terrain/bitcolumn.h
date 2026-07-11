// Fixed-width bit vector used to represent solid/air occupancy along one
// axis of a chunk. Lets face-exposure for an entire column be computed with
// a handful of word ops (`col & ~(col >> 1)`) instead of one comparison per
// voxel.
#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace BitOps {

template <size_t Bits>
struct BitColumn {
    static constexpr size_t WORDS = (Bits + 63) / 64;
    std::array<uint64_t, WORDS> w{};

    void set(int i) { w[i >> 6] |= (uint64_t(1) << (i & 63)); }
    bool test(int i) const { return (w[i >> 6] >> (i & 63)) & 1u; }

    // Bit i -> i+1 (equivalent to col << 1 across the whole vector).
    BitColumn shiftUp() const {
        BitColumn r;
        uint64_t carry = 0;
        for (size_t k = 0; k < WORDS; ++k) {
            r.w[k] = (w[k] << 1) | carry;
            carry = w[k] >> 63;
        }
        return r;
    }

    // Bit i -> i-1 (equivalent to col >> 1 across the whole vector).
    BitColumn shiftDown() const {
        BitColumn r;
        uint64_t carry = 0;
        for (size_t k = WORDS; k-- > 0;) {
            r.w[k] = (w[k] >> 1) | (carry << 63);
            carry = w[k] & 1u;
        }
        return r;
    }

    BitColumn operator&(const BitColumn& o) const {
        BitColumn r;
        for (size_t k = 0; k < WORDS; ++k) r.w[k] = w[k] & o.w[k];
        return r;
    }

    BitColumn operator|(const BitColumn& o) const {
        BitColumn r;
        for (size_t k = 0; k < WORDS; ++k) r.w[k] = w[k] | o.w[k];
        return r;
    }

    BitColumn operator~() const {
        BitColumn r;
        for (size_t k = 0; k < WORDS; ++k) r.w[k] = ~w[k];
        return r;
    }

    // Invoke fn(bitIndex) for every set bit, low to high, using
    // countr_zero to jump straight to each one instead of testing every
    // index in the vector.
    template <typename Fn>
    void forEachSetBit(Fn&& fn) const {
        for (size_t k = 0; k < WORDS; ++k) {
            uint64_t bits = w[k];
            while (bits) {
                int bit = std::countr_zero(bits);
                fn(static_cast<int>(k * 64) + bit);
                bits &= bits - 1; // clear lowest set bit
            }
        }
    }
};

}  // namespace BitOps
