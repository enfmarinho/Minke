/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef SIMD_H
#define SIMD_H

#include <cstdint>

#ifdef USE_AVX2

#include <immintrin.h>

constexpr int REGISTER_SIZE = 16;

using vepi16 = __m256i;
using vepi32 = __m256i;

inline vepi16 vepi16_zero() { return _mm256_setzero_si256(); }
inline vepi32 vepi32_zero() { return _mm256_setzero_si256(); }
inline vepi16 vepi16_set(const int16_t v) { return _mm256_set1_epi16(v); }
inline vepi16 vepi16_load(const int16_t* data) { return _mm256_load_si256(reinterpret_cast<const vepi16*>(data)); }
inline vepi16 vepi16_add(const vepi16 a, const vepi16 b) { return _mm256_add_epi16(a, b); }
inline vepi16 vepi16_sub(const vepi16 a, const vepi16 b) { return _mm256_sub_epi16(a, b); }
inline vepi16 vepi16_mult(const vepi16 a, const vepi16 b) { return _mm256_mullo_epi16(a, b); }
inline vepi32 vepi16_madd(const vepi16 a, const vepi16 b) { return _mm256_madd_epi16(a, b); }
inline vepi32 vepi32_add(const vepi32 a, const vepi32 b) { return _mm256_add_epi32(a, b); }

inline vepi16 vepi16_max(const vepi16 vec, const vepi16 max) { return _mm256_max_epi16(vec, max); }
inline vepi16 vepi16_min(const vepi16 vec, const vepi16 min) { return _mm256_min_epi16(vec, min); }
inline vepi16 vepi16_clamp(vepi16 vec, const vepi16 min, const vepi16 max) {
    return vepi16_max(vepi16_min(vec, max), min);
}
inline int32_t vepi32_reduce_add(const vepi32 vec) {
    // Add upper 128 bits and lower 128 bits together
    __m128i low = _mm256_castsi256_si128(vec);
    __m128i high = _mm256_extracti128_si256(vec, 1);
    __m128i sum128 = _mm_add_epi32(low, high);

    // Reduce sum128 downs to i32 using shuffle and add
    __m128i tmp = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2, 3, 0, 1)));
    tmp = _mm_add_epi32(tmp, _mm_shuffle_epi32(tmp, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm_cvtsi128_si32(tmp);
}

#elif USE_AVX512
#include <immintrin.h>
constexpr int REGISTER_SIZE = 32;

using vepi16 = __m512i;
using vepi32 = __m512i;

inline vepi16 vepi16_zero() { return _mm512_setzero_si512(); }
inline vepi32 vepi32_zero() { return _mm512_setzero_si512(); }
inline vepi16 vepi16_set(const int16_t v) { return _mm512_set1_epi16(v); }
inline vepi16 vepi16_load(const int16_t* data) { return _mm512_load_si512(reinterpret_cast<const vepi16*>(data)); }
inline vepi16 vepi16_add(const vepi16 a, const vepi16 b) { return _mm512_add_epi16(a, b); }
inline vepi16 vepi16_sub(const vepi16 a, const vepi16 b) { return _mm512_sub_epi16(a, b); }
inline vepi16 vepi16_mult(const vepi16 a, const vepi16 b) { return _mm512_mullo_epi16(a, b); }
inline vepi32 vepi16_madd(const vepi16 a, const vepi16 b) { return _mm512_madd_epi16(a, b); }
inline vepi32 vepi32_add(const vepi32 a, const vepi32 b) { return _mm512_add_epi32(a, b); }

inline vepi16 vepi16_max(const vepi16 vec, const vepi16 max) { return _mm512_max_epi16(vec, max); }
inline vepi16 vepi16_min(const vepi16 vec, const vepi16 min) { return _mm512_min_epi16(vec, min); }
inline vepi16 vepi16_clamp(vepi16 vec, const vepi16 min, const vepi16 max) {
    return vepi16_max(vepi16_min(vec, max), min);
}
inline int32_t vepi32_reduce_add(const vepi32 vec) { return _mm512_reduce_add_epi32(vec); }

#elif USE_NEON
#include <arm_neon.h>

constexpr int REGISTER_SIZE = 8;

using vepi16 = int16x8_t;
using vepi32 = int32x4_t;

inline vepi16 vepi16_zero() { return vdupq_n_s16(0); }
inline vepi32 vepi32_zero() { return vdupq_n_s32(0); }
inline vepi16 vepi16_set(const int16_t v) { return vdupq_n_s16(v); }
inline vepi16 vepi16_load(const int16_t* data) { return vld1q_s16(data); }
inline vepi16 vepi16_add(const vepi16 a, const vepi16 b) { return vaddq_s16(a, b); }
inline vepi16 vepi16_sub(const vepi16 a, const vepi16 b) { return vsubq_s16(a, b); }
inline vepi16 vepi16_mult(const vepi16 a, const vepi16 b) { return vmulq_s16(a, b); }
inline vepi32 vepi16_madd(const vepi16 a, const vepi16 b) {
    int32x4_t pl = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t ph = vmull_high_s16(a, b);
    return vpaddq_s32(pl, ph);
}
inline vepi32 vepi32_add(const vepi32 a, const vepi32 b) { return vaddq_s32(a, b); }
inline vepi16 vepi16_max(const vepi16 vec, const vepi16 max) { return vmaxq_s16(vec, max); }
inline vepi16 vepi16_min(const vepi16 vec, const vepi16 min) { return vminq_s16(vec, min); }
inline vepi16 vepi16_clamp(vepi16 vec, const vepi16 min, const vepi16 max) {
    return vepi16_max(vepi16_min(vec, max), min);
}
inline int32_t vepi32_reduce_add(const vepi32 vec) {
#if defined(__aarch64__)
    return vaddvq_s32(vec);
#else
    int32x2_t low = vget_low_s32(vec);
    int32x2_t high = vget_high_s32(vec);
    int32x2_t sum = vpadd_s32(low, high);
    sum = vpadd_s32(sum, sum);
    return vget_lane_s32(sum, 0);
#endif
}

#endif

#endif // #ifndef SIMD_H
