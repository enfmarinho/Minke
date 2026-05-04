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

#include "nnue.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>

#include "incbin.h"
#include "move.h"
#include "position.h"
#include "simd.h"
#include "types.h"
#include "utils.h"

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

void NNUE::pop() { m_accumulators.pop_back(); }

void NNUE::push(const DirtyPiece &dp) {
    m_accumulators.push_back(m_accumulators.back());
    switch (dp.move_type) {
        case REGULAR:
            add_sub(dp.add0, dp.sub0);
            break;
        case CAPTURE:
            add_sub2(dp.add0, dp.sub0, dp.sub1);
            break;
        case CASTLING:
            add2_sub2(dp.add0, dp.add1, dp.sub0, dp.sub1);
            break;
        default:
            __builtin_unreachable();
    }
}

void NNUE::add(const PieceSquare &ps) {
    const size_t white_index = ps.feature_idx(WHITE);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] += network.hidden_weights[white_index + column];
    }

    const size_t black_index = ps.feature_idx(BLACK);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] += network.hidden_weights[black_index + column];
    }
}

void NNUE::sub(const PieceSquare &ps) {
    const size_t white_index = ps.feature_idx(WHITE);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] -= network.hidden_weights[white_index + column];
    }

    const size_t black_index = ps.feature_idx(BLACK);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] -= network.hidden_weights[black_index + column];
    }
}

void NNUE::add_sub(const PieceSquare &add0, const PieceSquare &sub0) {
    const size_t white_add0_index = add0.feature_idx(WHITE);
    const size_t white_sub0_index = sub0.feature_idx(WHITE);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] +=
            network.hidden_weights[white_add0_index + column] - network.hidden_weights[white_sub0_index + column];
    }

    const size_t black_add0_index = add0.feature_idx(BLACK);
    const size_t black_sub0_index = sub0.feature_idx(BLACK);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] +=
            network.hidden_weights[black_add0_index + column] - network.hidden_weights[black_sub0_index + column];
    }
}

void NNUE::add_sub2(const PieceSquare &add0, const PieceSquare &sub0, const PieceSquare &sub1) {
    const size_t white_add0_index = add0.feature_idx(WHITE);
    const size_t white_sub0_index = sub0.feature_idx(WHITE);
    const size_t white_sub1_index = sub1.feature_idx(WHITE);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] += network.hidden_weights[white_add0_index + column] -
                                                       network.hidden_weights[white_sub0_index + column] -
                                                       network.hidden_weights[white_sub1_index + column];
    }

    const size_t black_add0_index = add0.feature_idx(BLACK);
    const size_t black_sub0_index = sub0.feature_idx(BLACK);
    const size_t black_sub1_index = sub1.feature_idx(BLACK);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] += network.hidden_weights[black_add0_index + column] -
                                                       network.hidden_weights[black_sub0_index + column] -
                                                       network.hidden_weights[black_sub1_index + column];
    }
}

void NNUE::add2_sub2(const PieceSquare &add0, const PieceSquare &add1, const PieceSquare &sub0,
                     const PieceSquare &sub1) {
    const size_t white_add0_index = add0.feature_idx(WHITE);
    const size_t white_add1_index = add1.feature_idx(WHITE);
    const size_t white_sub0_index = sub0.feature_idx(WHITE);
    const size_t white_sub1_index = sub1.feature_idx(WHITE);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] +=
            network.hidden_weights[white_add0_index + column] + network.hidden_weights[white_add1_index + column] -
            network.hidden_weights[white_sub0_index + column] - network.hidden_weights[white_sub1_index + column];
    }

    const size_t black_add0_index = add0.feature_idx(BLACK);
    const size_t black_add1_index = add1.feature_idx(BLACK);
    const size_t black_sub0_index = sub0.feature_idx(BLACK);
    const size_t black_sub1_index = sub1.feature_idx(BLACK);
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] +=
            network.hidden_weights[black_add0_index + column] + network.hidden_weights[black_add1_index + column] -
            network.hidden_weights[black_sub0_index + column] - network.hidden_weights[black_sub1_index + column];
    }
}

void NNUE::reset(const Position &position) {
    m_accumulators.clear();
    m_accumulators.emplace_back(network.hidden_bias);

    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);
        Piece piece = position.consult(sq);
        if (piece != EMPTY)
            add({piece, sq});
    }
}

constexpr int32_t NNUE::crelu(const int32_t &input) const { return std::clamp(input, CRELU_MIN, CRELU_MAX); }

constexpr int32_t NNUE::screlu(const int32_t &input) const {
    const int32_t crelu_out = crelu(input);
    return crelu_out * crelu_out;
}

ScoreType NNUE::flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &player,
                                          const std::array<int16_t, HIDDEN_LAYER_SIZE> &adversary) const {
#ifdef USE_SIMD
    vepi32 sum_vec = vepi32_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 player_weights_vec = vepi16_load(&network.output_weights[i]);
        vepi16 player_vec = vepi16_load(&player[i]);

        player_vec = vepi16_clamp(player_vec, QZERO, QONE);
        vepi32 player_product = vepi16_madd(vepi16_mult(player_vec, player_weights_vec), player_vec);
        sum_vec = vepi32_add(sum_vec, player_product);

        vepi16 adversary_weights_vec = vepi16_load(&network.output_weights[i + HIDDEN_LAYER_SIZE]);
        vepi16 adversary_vec = vepi16_load(&adversary[i]);

        adversary_vec = vepi16_clamp(adversary_vec, QZERO, QONE);
        vepi32 adversary_product = vepi16_madd(vepi16_mult(adversary_vec, adversary_weights_vec), adversary_vec);
        sum_vec = vepi32_add(sum_vec, adversary_product);
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(player[neuron_index]) * network.output_weights[neuron_index];
        sum += screlu(adversary[neuron_index]) * network.output_weights[neuron_index + HIDDEN_LAYER_SIZE];
    }

#endif

    sum = (sum / QA + network.output_bias) * SCALE / QAB;
    return std::clamp(sum, -MATE_FOUND + 1, MATE_FOUND - 1);
}

ScoreType NNUE::eval(const Color &stm) const {
    switch (stm) {
        case WHITE:
            return flatten_screlu_and_affine(m_accumulators.back().white_neurons, m_accumulators.back().black_neurons);
        case BLACK:
            return flatten_screlu_and_affine(m_accumulators.back().black_neurons, m_accumulators.back().white_neurons);
        default:
            assert(false && "Tried to use eval function with player none\n");
    }
    __builtin_unreachable();
}

NNUE::Accumulator::Accumulator(std::span<const int16_t, HIDDEN_LAYER_SIZE> bias) { reset(bias); }

void NNUE::Accumulator::reset(std::span<const int16_t, HIDDEN_LAYER_SIZE> bias) {
    memcpy(white_neurons.data(), bias.data(), bias.size_bytes());
    memcpy(black_neurons.data(), bias.data(), bias.size_bytes());
}

bool operator==(const NNUE::Accumulator &lhs, const NNUE::Accumulator &rhs) {
    for (size_t index = 0; index < lhs.black_neurons.size(); ++index) {
        if (lhs.black_neurons[index] != rhs.black_neurons[index] ||
            lhs.white_neurons[index] != rhs.white_neurons[index]) {
            return false;
        }
    }
    return true;
}

NNUE::Accumulator NNUE::debug_func(const Position &position) {
    Accumulator accumulator(network.hidden_bias);
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);
        Piece piece = position.consult(sq);
        PieceSquare ps = {piece, sq};
        if (piece != EMPTY) {
            const size_t white_index = ps.feature_idx(WHITE);
            const size_t black_index = ps.feature_idx(BLACK);

            for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
                accumulator.white_neurons[column] += network.hidden_weights[white_index * HIDDEN_LAYER_SIZE + column];
                accumulator.black_neurons[column] += network.hidden_weights[black_index * HIDDEN_LAYER_SIZE + column];
            }
        }
    }

    return accumulator;
}

const NNUE::Accumulator &NNUE::top() const { return m_accumulators.back(); }
