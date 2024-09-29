/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef NNUE_H
#define NNUE_H

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "game_elements.h"
#include "position.h"

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
  private:
    struct Accumulator;

  public:
    Network();
    ~Network() = default;

    void pop();
    void push();
    const Accumulator &top() const;

    void add_feature(const Square &sq, const PiecePlacement &pp);
    void remove_feature(const Square &sq, const PiecePlacement &pp);

    void reset(const Position &position);
    WeightType eval(const Player &stm) const;

    Accumulator debug_func(const Position &position);

  private:
    struct alignas(64) Parameters {
        std::array<int16_t, InputLayerSize * HiddenLayerSize> hidden_weights;
        std::array<int16_t, HiddenLayerSize> hidden_bias;
        std::array<int16_t, HiddenLayerSize * 2> output_weights;
        int16_t output_bias;
    };

    struct Accumulator {
        std::array<int16_t, HiddenLayerSize> white_neurons;
        std::array<int16_t, HiddenLayerSize> black_neurons;

        Accumulator(std::span<const int16_t, HiddenLayerSize> biasses);
        Accumulator() = delete;
        ~Accumulator() = default;
        inline void reset(std::span<const int16_t, HiddenLayerSize> biasses);
    };
    friend bool operator==(const Accumulator &lhs, const Accumulator &rhs);

    constexpr int32_t crelu(const int32_t &input) const;
    constexpr int32_t screlu(const int32_t &input) const;
    int32_t weight_sum_reduction(const std::array<int16_t, HiddenLayerSize> &player,
                                 const std::array<int16_t, HiddenLayerSize> &adversary) const;

    std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
    Parameters m_param;                      //!< Network parameters
};

#endif // #ifndef NNUE_H
