/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef NNUE_H
#define NNUE_H

#include "game_elements.h"
#include "position.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

constexpr int InputLayerSize = 64 * 12;
constexpr int HiddenLayerSize = 64;

// TODO check and fix quantization
class Network {
public:
  Network();
  ~Network() = default;

  void pop();
  void push();

  void add_feature(const Square &sq, const PiecePlacement &pp);
  void remove_feature(const Square &sq, const PiecePlacement &pp);

  void reset(const Position &position);
  WeightType evaluate(const Player &stm);

private:
  struct Accumulator {
    std::array<int16_t, HiddenLayerSize> white_neurons;
    std::array<int16_t, HiddenLayerSize> black_neurons;

    Accumulator(std::span<int16_t, HiddenLayerSize> biasses);
    Accumulator() = delete;
    ~Accumulator() = default;
    void reset(std::span<int16_t, HiddenLayerSize> biasses);
    int16_t *operator[](Player player);
  };

  int32_t crelu(const int16_t &input);

  std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
  std::array<std::array<int16_t, HiddenLayerSize>, InputLayerSize>
      m_hidden_weights;
  std::array<int16_t, HiddenLayerSize> m_hidden_bias;
  std::array<int16_t, HiddenLayerSize * 2> m_output_weights;
  int16_t m_output_bias;
};

#endif // #ifndef NNUE_H
