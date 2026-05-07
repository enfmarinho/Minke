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

#include "../../types.h"
#include "../../utils.h"
#include "simd.h"

constexpr int INPUT_LAYER_SIZE = 64 * 12;
constexpr int HIDDEN_LAYER_SIZE = 1024;

constexpr int NUM_KING_BUCKETS = 1;
constexpr int KING_BUCKETS_LAYOUT[64] = {0};

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

/// Check whether king has crossed half of the board, i.e. if the board should be flipped for horizontal mirroring
inline bool should_flip(const Square king_pov_sq) { return get_file(king_pov_sq) > 3; }

/// Input layer feature index
inline size_t feature_idx(const Piece piece, const Square sq, const Square king_pov_sq, const Color pov) {
    constexpr size_t COLOR_STRIDE = 64 * 6;
    constexpr size_t PIECE_STRIDE = 64;

    const Color piece_color = get_color(piece);
    int pov_sq = sq;
    if (pov == BLACK) // Convert square to pov, i.e. flip rank if stm is black
        pov_sq = pov_sq ^ 56;
    if (should_flip(king_pov_sq)) // Flip file
        pov_sq = pov_sq ^ 7;

    const size_t idx = KING_BUCKETS_LAYOUT[king_pov_sq] * INPUT_LAYER_SIZE + // Input bucket offset
                       (piece_color != pov) * COLOR_STRIDE +                 // Perspective offset
                       get_piece_type(piece, piece_color) * PIECE_STRIDE +   // Piece type offset
                       pov_sq;
    return idx * HIDDEN_LAYER_SIZE;
}
inline size_t feature_idx(const PieceSquare ps, const Square king_pov_sq, const Color pov) {
    return feature_idx(ps.piece, ps.sq, king_pov_sq, pov);
}

#endif // !ARCH_H
