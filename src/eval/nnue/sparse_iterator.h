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

#pragma once

#if USE_SIMD

#if USE_NEON
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "eval/nnue/arch.h"
#include "eval/nnue/simd.h"

class SparseIterator {
  public:
    inline size_t count() const { return m_count; }
    inline uint16_t chunk(size_t nnz_id) const { return m_chunks[nnz_id]; }

    void update(simd::vepu8 a, simd::vepu8 b);

  private:
#if USE_NEON
    using vepu16 = uint16x8_t;
    static inline vepu16 init_base() { return vdupq_n_u16(0); }
#elif USE_AVX2
    using vepu16 = __m128i;
    static inline vepu16 init_base() { return _mm_setzero_si128(); }
#elif USE_AVX512
    using vepu16 = __m512i;
    static inline vepu16 init_base() {
        return _mm512_set_epi16(            //
            31, 30, 29, 28, 27, 26, 25, 24, //
            23, 22, 21, 20, 19, 18, 17,     //
            16, 15, 14, 13, 12, 11, 10,     //
            9, 8, 7, 6, 5, 4, 3, 2, 1, 0    //
        );
    }
#else
#error "USE_SIMD is enabled, but no supported SIMD architecture is selected"
#endif

    alignas(64) uint16_t m_chunks[L1_SIZE / 4] = {};
    vepu16 m_base = init_base();
    size_t m_count = 0;
};

#else

// Not needed for scalar inference
class SparseIterator {};

#endif // #if USE_SIMD
