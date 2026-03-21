/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ARCH_H
#define ARCH_H

#include <array>
#include <cstdint>

#include "simd.h"

// clang-format off
constexpr int INPUT_BUCKETS = 1;
constexpr int KING_BUCKETS[64] = {0}; // No extra buckets for now
// clang-format on

constexpr int INPUT_LAYER_SIZE = 64 * 12;
constexpr int HIDDEN_LAYER_SIZE = 1024;

constexpr int32_t CRELU_MIN = 0;
constexpr int32_t CRELU_MAX = 255;

constexpr int SCALE = 400;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int QAB = QA * QB;

struct alignas(64) Network {
    std::array<int16_t, INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE> hidden_weights;
    std::array<int16_t, HIDDEN_LAYER_SIZE> hidden_bias;
    std::array<int16_t, HIDDEN_LAYER_SIZE * 2> output_weights;
    int16_t output_bias;
};
extern Network network;

#ifdef USE_SIMD
const vepi16 QZERO = vepi16_zero();
const vepi16 QONE = vepi16_set(QA);
#endif

#endif // #ifndef ARCH_H
