/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "eval/nnue/sparse_iterator.h"

#ifdef USE_SIMD

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "eval/nnue/simd.h"

#ifdef USE_AVX512

void SparseIterator::update(simd::vepu8 a, simd::vepu8 b) {
    using namespace simd;

    const auto mask = _mm512_kunpackw(nonzero_mask_u8(b), nonzero_mask_u8(a));
    const auto idxs = _mm512_maskz_compress_epi16(mask, m_base);

    _mm512_storeu_si512(&m_chunks[m_count], idxs);

    m_base = add_i16(m_base, set_i16(32));
    m_count += std::popcount(mask);
}

#else

namespace {

alignas(16) static constexpr auto nonzero_idx = []() {
    constexpr size_t COUNT = std::numeric_limits<uint8_t>::max() + 1;

    std::array<std::array<uint16_t, 8>, COUNT> idx{};

    for (size_t i = 0; i < COUNT; ++i) {
        size_t count = 0;

        for (uint8_t mask = i; mask != 0; mask &= mask - 1) {
            idx[i][count++] = std::countr_zero(mask);
        }
    }

    return idx;
}();

#if USE_NEON

using vep128u16 = uint16x8_t;

inline static vep128u16 set1(uint16_t v) { return vdupq_n_u16(v); }

inline static vep128u16 load(const void* ptr) { return vld1q_u16(reinterpret_cast<const uint16_t*>(ptr)); }

inline static void ustore(void* ptr, vep128u16 v) { return vst1q_u16(reinterpret_cast<uint16_t*>(ptr), v); }

inline static vep128u16 add(vep128u16 a, vep128u16 b) { return vaddq_u16(a, b); }

#elif USE_AVX2

using vep128u16 = __m128i;

inline static vep128u16 set1(uint16_t v) { return _mm_set1_epi16(static_cast<int16_t>(v)); }

inline static vep128u16 load(const void* ptr) { return _mm_load_si128(static_cast<const __m128i*>(ptr)); }

inline static void ustore(void* ptr, vep128u16 v) { _mm_storeu_si128(static_cast<__m128i*>(ptr), v); }

inline static vep128u16 add(vep128u16 a, vep128u16 b) { return _mm_add_epi16(a, b); }

#endif

} // namespace

void SparseIterator::update(simd::vepu8 a, simd::vepu8 b) {
    using namespace simd;
    const uint32_t full_mask = (nonzero_mask_u8(b) << CHUNK_SIZE_32BIT) | nonzero_mask_u8(a);

    for (uint32_t out = 0; out < CHUNK_SIZE_32BIT / 4; ++out) {
        const uint8_t mask = (full_mask >> (out * 8)) & 0xFF;

        const vepu16 nonzero = load(&nonzero_idx[mask]);
        const vepu16 idxs = add(m_base, nonzero);
        ustore(&m_chunks[m_count], idxs);

        m_base = add(m_base, set1(8));
        m_count += std::popcount(mask);
    }

    assert(m_count <= L1_SIZE / 4);
}

#endif

#endif // #if USE_SIMD
