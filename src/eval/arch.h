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

#ifndef ARCH_H
#define ARCH_H

#include <array>
#include <cstdint>

#include "simd.h"

constexpr int INPUT_LAYER_SIZE = 64 * 12;
constexpr int HIDDEN_LAYER_SIZE = 1024;

constexpr int32_t CRELU_MIN = 0;
constexpr int32_t CRELU_MAX = 255;

constexpr int SCALE = 400;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int QAB = QA * QB;

#ifdef USE_SIMD
const vepi16 QZERO = vepi16_zero();
const vepi16 QONE = vepi16_set(QA);
#endif

struct alignas(64) Network {
    std::array<int16_t, INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE> hidden_weights;
    std::array<int16_t, HIDDEN_LAYER_SIZE> hidden_bias;
    std::array<int16_t, HIDDEN_LAYER_SIZE * 2> output_weights;
    int16_t output_bias;
};
extern Network network;

#endif // #ifndef ARCH_H
