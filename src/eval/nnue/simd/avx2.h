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

#if USE_SIMD && USE_AVX2

#include <immintrin.h>

#include <cstddef>
#include <cstdint>

namespace simd {

constexpr size_t PACKUS_LANE_COUNT = 4;
constexpr size_t PACKUS_LANE_ORDER[4] = {0, 2, 1, 3};

using vepu8 = __m256i;
using vepu16 = __m256i;

using vepi8 = __m256i;
using vepi16 = __m256i;
using vepi32 = __m256i;

constexpr size_t CHUNK_SIZE_8BIT = sizeof(vepi8) / sizeof(int8_t);
constexpr size_t CHUNK_SIZE_16BIT = sizeof(vepi16) / sizeof(int16_t);
constexpr size_t CHUNK_SIZE_32BIT = sizeof(vepi32) / sizeof(int32_t);

/// u8
inline void store_u8(void* ptr, vepu8 v) { _mm256_store_si256(static_cast<vepu8*>(ptr), v); }

/// i8

inline vepi8 zero_i8() { return _mm256_setzero_si256(); }

inline vepi8 set_i8(const int8_t v) { return _mm256_set1_epi8(v); }

inline vepi8 load_i8(const void* ptr) { return _mm256_load_si256(static_cast<const vepi8*>(ptr)); }

inline void store_i8(void* ptr, vepi8 v) { _mm256_store_si256(static_cast<vepi8*>(ptr), v); }

inline vepi8 min_i8(vepi8 v, vepi8 min) { return _mm256_min_epi8(v, min); }

inline vepi8 max_i8(vepi8 v, vepi8 max) { return _mm256_max_epi8(v, max); }

inline vepi8 clamp_i8(vepi8 v, vepi8 min, vepi8 max) { return min_i8(max_i8(v, min), max); }

inline vepi8 add_i8(vepi8 a, vepi8 b) { return _mm256_add_epi8(a, b); }

inline vepi8 sub_i8(vepi8 a, vepi8 b) { return _mm256_sub_epi8(a, b); }

/// i16
inline vepi16 zero_i16() { return _mm256_setzero_si256(); }

inline vepi16 set_i16(const int16_t v) { return _mm256_set1_epi16(v); }

inline vepi16 load_i16(const void* ptr) { return _mm256_load_si256(static_cast<const vepi16*>(ptr)); }

inline void store_i16(void* ptr, vepi16 v) { _mm256_store_si256(static_cast<vepi16*>(ptr), v); }

inline vepi16 min_i16(const vepi16 v, const vepi16 min) { return _mm256_min_epi16(v, min); }

inline vepi16 max_i16(const vepi16 v, const vepi16 max) { return _mm256_max_epi16(v, max); }

inline vepi16 clamp_i16(vepi16 vec, const vepi16 min, const vepi16 max) { return min_i16(max_i16(vec, min), max); }

inline vepi16 add_i16(const vepi16 a, const vepi16 b) { return _mm256_add_epi16(a, b); }

inline vepi16 sub_i16(const vepi16 a, const vepi16 b) { return _mm256_sub_epi16(a, b); }

inline vepi16 mullo_i16(const vepi16 a, const vepi16 b) { return _mm256_mullo_epi16(a, b); }

inline vepi16 mulhi_i16(const vepi16 a, const vepi16 b) { return _mm256_mulhi_epi16(a, b); }

inline vepi32 madd_i16(const vepi16 a, const vepi16 b) { return _mm256_madd_epi16(a, b); }

inline vepi16 shiftleft_i16(const vepi16 a, const int c) { return _mm256_slli_epi16(a, c); }

inline vepi16 shiftright_i16(const vepi16 a, const int c) { return _mm256_srai_epi16(a, c); }

inline vepu8 packus_i16(const vepi16 a, const vepi16 b) {
    vepu8 packed = _mm256_packus_epi16(a, b);
    return _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
}

/// i32

inline vepi32 zero_i32() { return _mm256_setzero_si256(); }

inline vepi32 set_i32(const int32_t v) { return _mm256_set1_epi32(v); }

inline vepi32 load_i32(const void* ptr) { return _mm256_load_si256(static_cast<const vepi32*>(ptr)); }

inline void store_i32(void* ptr, vepi32 a) { _mm256_store_si256(static_cast<vepi32*>(ptr), a); }

inline vepi32 min_i32(vepi32 v, vepi32 min) { return _mm256_min_epi32(v, min); }

inline vepi32 max_i32(vepi32 v, vepi32 max) { return _mm256_max_epi32(v, max); }

inline vepi32 clamp_i32(vepi32 v, vepi32 min, vepi32 max) { return min_i32(max_i32(v, min), max); }

inline vepi32 add_i32(vepi32 a, vepi32 b) { return _mm256_add_epi32(a, b); }

inline vepi32 sub_i32(vepi32 a, vepi32 b) { return _mm256_sub_epi32(a, b); }

inline vepi32 mullo_i32(vepi32 a, vepi32 b) { return _mm256_mullo_epi32(a, b); }

inline vepi32 shiftleft_i32(const vepi32 a, const int c) { return _mm256_slli_epi32(a, c); }

inline vepi32 shiftright_i32(const vepi32 a, const int c) { return _mm256_srai_epi32(a, c); }

inline int32_t hsum_i32(const vepi32 vec) {
    // Add upper 128 bits and lower 128 bits together
    __m128i low = _mm256_castsi256_si128(vec);
    __m128i high = _mm256_extracti128_si256(vec, 1);
    __m128i sum128 = _mm_add_epi32(low, high);

    // Reduce sum128 downs to i32 using shuffle and add
    __m128i tmp = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2, 3, 0, 1)));
    tmp = _mm_add_epi32(tmp, _mm_shuffle_epi32(tmp, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm_cvtsi128_si32(tmp);
}

inline vepi32 dpbusd_i32(vepi32 sum, vepu8 u, vepi8 i) {
    const vepi16 p = _mm256_maddubs_epi16(u, i);
    const vepi32 w = madd_i16(p, set_i16(1));
    return add_i32(sum, w);
}

} // namespace simd

#endif
