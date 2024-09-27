/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "nnue.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

#include "game_elements.h"
#include "incbin.h"
#include "position.h"

INCBIN(NetParameters, "../src/minke.bin");

Network::Network() {
    const int16_t *pointer = reinterpret_cast<const int16_t *>(gNetParametersData);

static const std::pair<size_t, size_t> feature_indices(const Square &sq, const PiecePlacement &pp) {
    constexpr size_t ColorStride = 64 * 6;
    constexpr size_t PieceStride = 64;

    int nnue_index = MailboxToStandardNNUE[pp.index()];

    size_t white_index = (sq.player == Player::Black ? ColorStride : 0) + piece_index(sq.piece) * PieceStride +
                         static_cast<size_t>(nnue_index ^ 56);
    size_t black_index = (sq.player == Player::White ? ColorStride : 0) + piece_index(sq.piece) * PieceStride +
                         static_cast<size_t>(nnue_index);
    return {white_index, black_index};
}

}

void Network::pop() { m_accumulators.pop_back(); }

void Network::push() { m_accumulators.push_back(m_accumulators.back()); }

void Network::add_feature(const Square &sq, const PiecePlacement &pp) {
    const auto [white_index, black_index] = feature_indices(sq, pp);

    for (int column{0}; column < HiddenLayerSize; ++column) {
        m_accumulators.back().white_neurons[column] += m_hidden_weights[white_index][column];
        m_accumulators.back().black_neurons[column] += m_hidden_weights[black_index][column];
    }
}

void Network::remove_feature(const Square &sq, const PiecePlacement &pp) {
    const auto [white_index, black_index] = feature_indices(sq, pp);

    for (int column{0}; column < HiddenLayerSize; ++column) {
        m_accumulators.back().white_neurons[column] -= m_hidden_weights[white_index][column];
        m_accumulators.back().black_neurons[column] -= m_hidden_weights[black_index][column];
    }
}

void Network::reset(const Position &position) {
    m_accumulators.clear();
    m_accumulators.emplace_back(m_hidden_bias);

    for (IndexType file = 0; file < BoardHeight; ++file) {
        for (IndexType rank = 0; rank < BoardWidth; ++rank) {
            PiecePlacement pp(file, rank);
            const Square &sq = position.consult(pp);
            if (sq.piece != Piece::None) {
                add_feature(sq, pp);
            }
        }
    }
}

int32_t Network::crelu(const int32_t &input) const { return std::clamp(input, CreluMin, CreluMax); }

int32_t Network::screlu(const int32_t &input) const {
    const int32_t crelu_out = crelu(input);
    return crelu_out * crelu_out;
}

int32_t Network::weight_sum_reduction(const std::array<int32_t, HiddenLayerSize> &player,
                                      const std::array<int32_t, HiddenLayerSize> &adversary) const {
    int32_t eval = 0;
    for (int neuron_index = 0; neuron_index < HiddenLayerSize; ++neuron_index) {
        eval += screlu(player[neuron_index]) * m_output_weights[neuron_index];
        eval += screlu(adversary[neuron_index]) * m_output_weights[neuron_index + HiddenLayerSize];
    }
    return (eval / QA + m_output_bias) * Scale / QAB;
}

WeightType Network::eval(const Player &stm) const {
    switch (stm) {
        case Player::White:
            return weight_sum_reduction(m_accumulators.back().white_neurons, m_accumulators.back().black_neurons);
        case Player::Black:
            return weight_sum_reduction(m_accumulators.back().black_neurons, m_accumulators.back().white_neurons);
        default:
            std::cerr << "Tried to use eval function with player none\n";
            assert(false); // Must not reach this
    }
    return WeightType(); // Can't reach this, just to avoid compiler warnings
}

Network::Accumulator::Accumulator(std::span<int32_t, HiddenLayerSize> bias) { reset(bias); }

void Network::Accumulator::reset(std::span<int32_t, HiddenLayerSize> bias) {
    memcpy(white_neurons.data(), bias.data(), bias.size_bytes());
    memcpy(black_neurons.data(), bias.data(), bias.size_bytes());
}

bool operator==(const Network::Accumulator &lhs, const Network::Accumulator &rhs) {
    for (size_t index = 0; index < lhs.black_neurons.size(); ++index) {
        if (lhs.black_neurons[index] != rhs.black_neurons[index] ||
            lhs.white_neurons[index] != rhs.white_neurons[index]) {
            return false;
        }
    }
    return true;
}

Network::Accumulator Network::debug_func(const Position &position) {
    Accumulator accumulator(m_hidden_bias);
    for (IndexType file = 0; file < BoardHeight; ++file) {
        for (IndexType rank = 0; rank < BoardWidth; ++rank) {
            PiecePlacement pp(file, rank);
            const Square &sq = position.consult(pp);
            if (sq.piece != Piece::None) {
                const auto [white_index, black_index] = feature_indices(sq, pp);

                for (int column{0}; column < HiddenLayerSize; ++column) {
                    accumulator.white_neurons[column] += m_hidden_weights[white_index][column];
                    accumulator.black_neurons[column] += m_hidden_weights[black_index][column];
                }
            }
        }
    }

    return accumulator;
}

const Network::Accumulator &Network::top() const { return m_accumulators.back(); }
