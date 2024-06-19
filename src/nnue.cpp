/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "nnue.h"
#include "game_elements.h"
#include "position.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <span>

IndexType feature_index(const Square &sq, const PiecePlacement &pp) {
  return (pp.rank() * 8 + pp.file()) +
         64 * (piece_index(sq.piece) +
               (sq.player == Player::Black ? NumberOfPieces : 0));
}

Network::Network() {
  // TODO read NN parameters
}

void Network::pop() { m_accumulators.pop_back(); }

void Network::push() { m_accumulators.push_back(m_accumulators.back()); }

void Network::add_feature(const Square &sq, const PiecePlacement &pp) {
  IndexType white_index = feature_index(sq, pp);
  IndexType black_index = feature_index(sq, pp.mirrored());

  for (int column{0}; column < HiddenLayerSize; ++column) {
    m_accumulators.back().white_neurons[column] +=
        m_hidden_weights[white_index][column];
    m_accumulators.back().black_neurons[column] +=
        m_hidden_weights[black_index][column];
  }
}

void Network::remove_feature(const Square &sq, const PiecePlacement &pp) {
  IndexType white_index = feature_index(sq, pp);
  IndexType black_index = feature_index(sq, pp.mirrored());

  for (int column{0}; column < HiddenLayerSize; ++column) {
    m_accumulators.back().white_neurons[column] -=
        m_hidden_weights[white_index][column];
    m_accumulators.back().black_neurons[column] -=
        m_hidden_weights[black_index][column];
  }
}

void Network::reset(const Position &position) {
  m_accumulators.clear();
  m_accumulators.emplace_back(m_hidden_bias);

  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement pp(file, rank);
      add_feature(position.consult(pp), pp);
    }
  }
}

int32_t Network::crelu(const int16_t &input) const {
  return int32_t(); // TODO just a STUB
}

int32_t Network::weight_sum_reduction(
    const std::array<int16_t, HiddenLayerSize> &player,
    const std::array<int16_t, HiddenLayerSize> &adversary) const {
  int32_t eval = 0;
  for (int neuron_index = 0; neuron_index < HiddenLayerSize; ++neuron_index) {
    eval += crelu(player[neuron_index]) * m_output_weights[neuron_index];
    eval += crelu(adversary[neuron_index]) *
            m_output_weights[neuron_index + HiddenLayerSize];
  }
  return eval + m_output_bias; // TODO check quantization
}

WeightType Network::eval(const Player &stm) const {
  switch (stm) {
  case Player::White:
    return weight_sum_reduction(m_accumulators.back().white_neurons,
                                m_accumulators.back().black_neurons);
  case Player::Black:
    return weight_sum_reduction(m_accumulators.back().black_neurons,
                                m_accumulators.back().white_neurons);
  default:
    assert(false); // Must not reach this
  }
}

Network::Accumulator::Accumulator(std::span<int16_t, HiddenLayerSize> bias) {
  reset(bias);
}

void Network::Accumulator::reset(std::span<int16_t, HiddenLayerSize> bias) {
  memcpy(white_neurons.data(), bias.data(), bias.size_bytes());
  memcpy(black_neurons.data(), bias.data(), bias.size_bytes());
}

int16_t *Network::Accumulator::operator[](Player player) {
  switch (player) {
  case Player::White:
    return white_neurons.data();
    break;
  case Player::Black:
    return black_neurons.data();
    break;
  default:
    assert(false);
    break;
  }
}
