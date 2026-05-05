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

#include "pov_accumulator.h"

#include <cstddef>

#include "../../utils.h"

size_t PieceSquare::feature_idx(const Color pov) const {
    constexpr size_t COLOR_STRIDE = 64 * 6;
    constexpr size_t PIECE_STRIDE = 64;

    const Color piece_color = get_color(piece);
    int pov_sq = sq;
    if (pov == BLACK) // Convert square to pov, i.e. flip rank if stm is black
        pov_sq = pov_sq ^ 56;

    const size_t idx = (piece_color != pov) * COLOR_STRIDE +               // Perspective offset
                       get_piece_type(piece, piece_color) * PIECE_STRIDE + // Piece type offset
                       pov_sq;
    return idx * HIDDEN_LAYER_SIZE;
}

void PovAccumulator::add(const PovAccumulator &input, const size_t feature_idx) {
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.hidden_weights[feature_idx + column];
    }
}

void PovAccumulator::add_sub(const PovAccumulator &input, const size_t add0, const size_t sub0) {
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_neurons[column] =
            input.m_neurons[column] + network.hidden_weights[add0 + column] - network.hidden_weights[sub0 + column];
    }
}

void PovAccumulator::add_sub2(const PovAccumulator &input, const size_t add0, const size_t sub0, const size_t sub1) {
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.hidden_weights[add0 + column] -
                            network.hidden_weights[sub0 + column] - network.hidden_weights[sub1 + column];
    }
}

void PovAccumulator::add2_sub2(const PovAccumulator &input, const size_t add0, const size_t add1, const size_t sub0,
                               const size_t sub1) {
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.hidden_weights[add0 + column] +
                            network.hidden_weights[add1 + column] - network.hidden_weights[sub0 + column] -
                            network.hidden_weights[sub1 + column];
    }
}

void PovAccumulator::self_add(const size_t feature_idx) { add(*this, feature_idx); }
