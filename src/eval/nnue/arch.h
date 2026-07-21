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

#include <cstdint>

#include "../../types.h"
#include "../../utils.h"

constexpr int INPUT_LAYER_SIZE = 64 * 12;
constexpr int L1_SIZE = 1024;
constexpr int L2_SIZE = 16;
constexpr int L3_SIZE = 32;

constexpr int NUM_KING_BUCKETS = 10;
// clang-format off
constexpr int KING_BUCKETS_LAYOUT[64] = {
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 5, 6, 7, 7, 6, 5, 4,
    8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,

};
// clang-format on

constexpr int OUTPUT_BUCKET_COUNT = 8;
constexpr int BUCKET_SIZE = 32 / OUTPUT_BUCKET_COUNT;

constexpr int32_t CRELU_MIN = 0;
constexpr int32_t CRELU_MAX = 255;

constexpr int32_t SCALE = 400;

constexpr int32_t QA = 255;
constexpr int32_t QB = 128;
constexpr int32_t QC = 64;

constexpr int32_t FT_SCALE_BITS = 7;

struct alignas(64) Network {
    int16_t ft_weights[NUM_KING_BUCKETS * INPUT_LAYER_SIZE * L1_SIZE];
    int16_t ft_biases[L1_SIZE];
    int8_t l1_weights[OUTPUT_BUCKET_COUNT][L1_SIZE / 4][L2_SIZE][4];
    int32_t l1_biases[OUTPUT_BUCKET_COUNT][L2_SIZE];
    int32_t l2_weights[OUTPUT_BUCKET_COUNT][L2_SIZE][L3_SIZE];
    int32_t l2_biases[OUTPUT_BUCKET_COUNT][L3_SIZE];
    int32_t l3_weights[OUTPUT_BUCKET_COUNT][L3_SIZE];
    int32_t l3_biases[OUTPUT_BUCKET_COUNT];
};
extern Network network;

/// Check whether king has crossed half of the board, i.e. if the board should be flipped for horizontal mirroring
inline bool should_flip(const Square king_sq) { return get_file(king_sq) > 3; }

/// Consult king bucket index
inline size_t king_bucket_idx(const Square king_sq, const Color pov) {
    // Convert square to pov, i.e. flip rank if stm is black
    const size_t king_pov_sq = king_sq ^ (pov == BLACK ? 56 : 0);
    return KING_BUCKETS_LAYOUT[king_pov_sq];
}

/// Input layer feature index
inline size_t feature_idx(const Piece piece, const Square sq, const Square king_sq, const Color pov) {
    constexpr size_t COLOR_STRIDE = 64 * 6;
    constexpr size_t PIECE_STRIDE = 64;

    const Color piece_color = get_color(piece);
    int pov_sq = sq;
    if (pov == BLACK) { // Convert square to pov, i.e. flip rank if stm is black
        pov_sq ^= 56;
    }
    if (should_flip(king_sq)) // Flip file
        pov_sq = pov_sq ^ 7;

    const size_t idx = king_bucket_idx(king_sq, pov) * INPUT_LAYER_SIZE +  // Input bucket offset
                       (piece_color != pov) * COLOR_STRIDE +               // Perspective offset
                       get_piece_type(piece, piece_color) * PIECE_STRIDE + // Piece type offset
                       pov_sq;
    return idx * L1_SIZE;
}

inline size_t feature_idx(const PieceSquare ps, const Square king_sq, const Color pov) {
    return feature_idx(ps.piece, ps.sq, king_sq, pov);
}

#endif // !ARCH_H
