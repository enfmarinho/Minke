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

static int feature_index(const Square &sq, const PiecePlacement &pp) {
    return (pp.rank() * 8 + pp.file()) +
           64 * (piece_index(sq.piece) + (sq.player == Player::Black ? NumberOfPieces : 0));
}

Network::Network() {
    const int16_t *pointer = reinterpret_cast<const int16_t *>(gNetParametersData);

    for (int j = 0; j < HiddenLayerSize; ++j)
        for (int i = 0; i < InputLayerSize; ++i)
            m_hidden_weights[j][i] = *pointer++;

    for (int i = 0; i < HiddenLayerSize; ++i)
        m_hidden_bias[i] = *pointer++;

    for (int i = 0; i < HiddenLayerSize * 2; ++i)
        m_output_weights[i] = *pointer++;

    m_output_bias = *pointer++;

    // assert(reinterpret_cast<const unsigned char *>(pointer) ==
    //        gNetParametersData + gNetParametersSize);
}

void Network::pop() { m_accumulators.pop_back(); }

void Network::push() { m_accumulators.push_back(m_accumulators.back()); }

void Network::add_feature(const Square &sq, const PiecePlacement &pp) {
    int white_index = feature_index(sq, pp);
    int black_index = feature_index(sq, pp);

    for (int column{0}; column < HiddenLayerSize; ++column) {
        m_accumulators.back().white_neurons[column] += m_hidden_weights[column][white_index];
        m_accumulators.back().black_neurons[column] += m_hidden_weights[column][black_index];
    }
}

void Network::remove_feature(const Square &sq, const PiecePlacement &pp) {
    int white_index = feature_index(sq, pp);
    int black_index = feature_index(sq, pp);

    for (int column{0}; column < HiddenLayerSize; ++column) {
        m_accumulators.back().white_neurons[column] -= m_hidden_weights[column][white_index];
        m_accumulators.back().black_neurons[column] -= m_hidden_weights[column][black_index];
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
                int white_index = feature_index(sq, pp);
                int black_index = feature_index(sq, pp);

                for (int column{0}; column < HiddenLayerSize; ++column) {
                    accumulator.white_neurons[column] += m_hidden_weights[column][white_index];
                    accumulator.black_neurons[column] += m_hidden_weights[column][black_index];
                }
            }
        }
    }

    return accumulator;
}

const Network::Accumulator &Network::top() const { return m_accumulators.back(); }
