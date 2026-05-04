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

static const std::pair<size_t, size_t> feature_indices(const Piece &piece, const Square &sq) {
    constexpr size_t ColorStride = 64 * 6;
    constexpr size_t PieceStride = 64;

    Color color = get_color(piece);
    PieceType piece_type = get_piece_type(piece, color);

    size_t white_index = color * ColorStride + piece_type * PieceStride + sq;
    size_t black_index = !color * ColorStride + piece_type * PieceStride + (sq ^ 56);
    return {white_index, black_index};
}

void NNUE::pop() { m_accumulators.pop_back(); }

void NNUE::push(const DirtyPiece &dp) {
    m_accumulators.push_back(m_accumulators.back());
    switch (dp.move_type) {
        case REGULAR:
            remove_feature(dp.sub0);
            add_feature(dp.add0);
            break;
        case CAPTURE:
            remove_feature(dp.sub0);
            remove_feature(dp.sub1);
            add_feature(dp.add0);
            break;
        case CASTLING:
            remove_feature(dp.sub0);
            remove_feature(dp.sub1);
            add_feature(dp.add0);
            add_feature(dp.add1);
            break;
        default:
            __builtin_unreachable();
    }
}

void NNUE::add_feature(const PieceSquare &ps) {
    const auto [white_index, black_index] = feature_indices(ps.piece, ps.sq);

    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] += network.hidden_weights[white_index * HIDDEN_LAYER_SIZE + column];
    }
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] += network.hidden_weights[black_index * HIDDEN_LAYER_SIZE + column];
    }
}

void NNUE::remove_feature(const PieceSquare &ps) {
    const auto [white_index, black_index] = feature_indices(ps.piece, ps.sq);

    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().white_neurons[column] -= network.hidden_weights[white_index * HIDDEN_LAYER_SIZE + column];
    }
    for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
        m_accumulators.back().black_neurons[column] -= network.hidden_weights[black_index * HIDDEN_LAYER_SIZE + column];
    }
}

void NNUE::reset(const Position &position) {
    m_accumulators.clear();
    m_accumulators.emplace_back(network.hidden_bias);

    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);
        Piece piece = position.consult(sq);
        if (piece != EMPTY)
            add_feature({piece, sq});
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
        if (piece != EMPTY) {
            const auto [white_index, black_index] = feature_indices(piece, sq);

            for (int column{0}; column < HIDDEN_LAYER_SIZE; ++column) {
                accumulator.white_neurons[column] += network.hidden_weights[white_index * HIDDEN_LAYER_SIZE + column];
                accumulator.black_neurons[column] += network.hidden_weights[black_index * HIDDEN_LAYER_SIZE + column];
            }
        }
    }

    return accumulator;
}

const NNUE::Accumulator &NNUE::top() const { return m_accumulators.back(); }
