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

#if USE_SIMD && USE_NEON

#include <arm_neon.h>

#include <cstddef>
#include <cstdint>

namespace simd {

constexpr size_t PACKUS_LANE_COUNT = 2;
constexpr size_t PACKUS_LANE_ORDER[2] = {0, 1};

using vepu8 = uint8x16_t;
using vepu16 = uint16x8_t;

using vepi8 = int8x16_t;
using vepi16 = int16x8_t;
using vepi32 = int32x4_t;

constexpr size_t CHUNK_SIZE_8BIT = sizeof(vepi8) / sizeof(int8_t);
constexpr size_t CHUNK_SIZE_16BIT = sizeof(vepi16) / sizeof(int16_t);
constexpr size_t CHUNK_SIZE_32BIT = sizeof(vepi32) / sizeof(int32_t);

/// u8
inline void store_u8(void* ptr, vepu8 v) { vst1q_u8(static_cast<uint8_t*>(ptr), v); }

/// i8

inline vepi8 zero_i8() { return vdupq_n_s8(0); }

inline vepi8 set_i8(const int8_t v) { return vdupq_n_s8(v); }

inline vepi8 load_i8(const void* ptr) { return vld1q_s8(static_cast<const int8_t*>(ptr)); }

inline void store_i8(void* ptr, vepi8 v) { vst1q_s8(static_cast<int8_t*>(ptr), v); }

inline vepi8 min_i8(vepi8 v, vepi8 min) { return vminq_s8(v, min); }

inline vepi8 max_i8(vepi8 v, vepi8 max) { return vmaxq_s8(v, max); }

inline vepi8 clamp_i8(vepi8 v, vepi8 min, vepi8 max) { return min_i8(max_i8(v, min), max); }

inline vepi8 add_i8(vepi8 a, vepi8 b) { return vaddq_s8(a, b); }

inline vepi8 sub_i8(vepi8 a, vepi8 b) { return vsubq_s8(a, b); }

/// i16
inline vepi16 zero_i16() { return vdupq_n_s16(0); }

inline vepi16 set_i16(const int16_t v) { return vdupq_n_s16(v); }

inline vepi16 load_i16(const void* ptr) { return vld1q_s16(static_cast<const int16_t*>(ptr)); }

inline void store_i16(void* ptr, vepi16 v) { vst1q_s16(static_cast<int16_t*>(ptr), v); }

inline vepi16 min_i16(const vepi16 v, const vepi16 min) { return vminq_s16(v, min); }

inline vepi16 max_i16(const vepi16 v, const vepi16 max) { return vmaxq_s16(v, max); }

inline vepi16 clamp_i16(vepi16 vec, const vepi16 min, const vepi16 max) { return min_i16(max_i16(vec, min), max); }

inline vepi16 add_i16(const vepi16 a, const vepi16 b) { return vaddq_s16(a, b); }

inline vepi16 sub_i16(const vepi16 a, const vepi16 b) { return vsubq_s16(a, b); }

inline vepi16 mullo_i16(const vepi16 a, const vepi16 b) { return vmulq_s16(a, b); }

inline vepi16 mulhi_i16(const vepi16 a, const vepi16 b) {
    int32x4_t low = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t high = vmull_s16(vget_high_s16(a), vget_high_s16(b));
    return vcombine_s16(vshrn_n_s32(low, 16), vshrn_n_s32(high, 16));
}

inline vepi32 madd_i16(const vepi16 a, const vepi16 b) {
    int32x4_t low = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t high = vmull_s16(vget_high_s16(a), vget_high_s16(b));
    return vpaddq_s32(low, high);
}

inline vepi16 shiftleft_i16(const vepi16 a, const int c) { return vshlq_s16(a, vdupq_n_s16(static_cast<int16_t>(c))); }

inline vepi16 shiftright_i16(const vepi16 a, const int c) {
    return vshlq_s16(a, vdupq_n_s16(static_cast<int16_t>(-c)));
}

inline vepu8 packus_i16(const vepi16 a, const vepi16 b) {
    uint8x8_t low = vqmovun_s16(a);
    uint8x8_t high = vqmovun_s16(b);
    return vcombine_u8(low, high);
}

/// i32

inline vepi32 zero_i32() { return vdupq_n_s32(0); }

inline vepi32 set_i32(const int32_t v) { return vdupq_n_s32(v); }

inline vepi32 load_i32(const void* ptr) { return vld1q_s32(static_cast<const int32_t*>(ptr)); }

inline void store_i32(void* ptr, vepi32 a) { vst1q_s32(static_cast<int32_t*>(ptr), a); }

inline vepi32 min_i32(vepi32 v, vepi32 min) { return vminq_s32(v, min); }

inline vepi32 max_i32(vepi32 v, vepi32 max) { return vmaxq_s32(v, max); }

inline vepi32 clamp_i32(vepi32 v, vepi32 min, vepi32 max) { return min_i32(max_i32(v, min), max); }

inline vepi32 add_i32(vepi32 a, vepi32 b) { return vaddq_s32(a, b); }

inline vepi32 sub_i32(vepi32 a, vepi32 b) { return vsubq_s32(a, b); }

inline vepi32 mullo_i32(vepi32 a, vepi32 b) { return vmulq_s32(a, b); }

inline vepi32 shiftleft_i32(const vepi32 a, const int c) { return vshlq_s32(a, vdupq_n_s32(c)); }

inline vepi32 shiftright_i32(const vepi32 a, const int c) { return vshlq_s32(a, vdupq_n_s32(-c)); }

inline vepu16 packus_i32(const vepi32 a, const vepi32 b) {
    uint16x4_t low = vqmovun_s32(a);
    uint16x4_t high = vqmovun_s32(b);
    return vcombine_u16(low, high);
}

inline int32_t hsum_i32(const vepi32 vec) {
#if defined(__aarch64__)
    return vaddvq_s32(vec);
#else
    int32x2_t sum64 = vadd_s32(vget_low_s32(vec), vget_high_s32(vec));
    return vget_lane_s32(vpadd_s32(sum64, sum64), 0);
#endif
}

inline vepi32 dpbusd_i32(vepi32 sum, vepu8 u, vepi8 i) {
#if defined(__ARM_FEATURE_I8MM)
    return vusdotq_s32(sum, u, i);
#else
    int16x8_t p_low = vmulq_s16(vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(u))), vmovl_s8(vget_low_s8(i)));
    int16x8_t p_high = vmulq_s16(vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(u))), vmovl_s8(vget_high_s8(i)));

    int16x8_t pair_sum = vpaddq_s16(p_low, p_high);
    int32x4_t quad_sum = vpaddlq_s16(pair_sum);

    return vaddq_s32(sum, quad_sum);
#endif
}

} // namespace simd

#endif
