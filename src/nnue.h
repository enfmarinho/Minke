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
constexpr int HiddenLayerSize = 64 * 12;

constexpr int32_t CreluMin = 0;
constexpr int32_t CreluMax = 255;

constexpr int Scale = 400;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int QAB = QA * QB;

// NOTE: Network must be initialized using reset().
class Network {
public:
  Network();
  ~Network() = default;

  void pop();
  void push();

  void add_feature(const Square &sq, const PiecePlacement &pp);
  void remove_feature(const Square &sq, const PiecePlacement &pp);

  void reset(const Position &position);
  WeightType eval(const Player &stm) const;

private:
  struct Accumulator {
    std::array<int32_t, HiddenLayerSize> white_neurons;
    std::array<int32_t, HiddenLayerSize> black_neurons;

    Accumulator(std::span<int32_t, HiddenLayerSize> biasses);
    Accumulator() = delete;
    ~Accumulator() = default;
    void reset(std::span<int32_t, HiddenLayerSize> biasses);
  };

private:
  int32_t crelu(const int32_t &input) const;
  int32_t screlu(const int32_t &input) const;
  int32_t weight_sum_reduction(
      const std::array<int32_t, HiddenLayerSize> &player,
      const std::array<int32_t, HiddenLayerSize> &adversary) const;

  std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
  std::array<std::array<int32_t, InputLayerSize>, HiddenLayerSize>
      m_hidden_weights;
  std::array<int32_t, HiddenLayerSize> m_hidden_bias;
  std::array<int32_t, HiddenLayerSize * 2> m_output_weights;
  int32_t m_output_bias;
};

#endif // #ifndef NNUE_H
