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
#include <span>
#include <vector>

#include "simd.h"
#include "types.h"

class Position;

constexpr int INPUT_LAYER_SIZE = 64 * 12;
constexpr int HIDDEN_LAYER_SIZE = 1024;

constexpr int32_t CRELU_MIN = 0;
constexpr int32_t CRELU_MAX = 255;

constexpr int SCALE = 400;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int QAB = QA * QB;

#ifdef USE_SIMD
const vepi16 QZERO = vepi16_zero();
const vepi16 QONE = vepi16_set(QA);
#endif

struct alignas(64) Network {
    std::array<int16_t, INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE> hidden_weights;
    std::array<int16_t, HIDDEN_LAYER_SIZE> hidden_bias;
    std::array<int16_t, HIDDEN_LAYER_SIZE * 2> output_weights;
    int16_t output_bias;
};
extern Network network;

// NOTE: NNUE must be initialized using reset().
class NNUE {
  private:
    struct Accumulator;

  public:
    NNUE() = default;
    ~NNUE() = default;

    void pop();
    void push();
    const Accumulator &top() const;

    void add_feature(const Piece &piece, const Square &sq);
    void remove_feature(const Piece &piece, const Square &sq);

    void reset(const Position &position);
    ScoreType eval(const Color &stm) const;

    Accumulator debug_func(const Position &position);

  private:
    struct alignas(64) Accumulator {
        std::array<int16_t, HIDDEN_LAYER_SIZE> white_neurons;
        std::array<int16_t, HIDDEN_LAYER_SIZE> black_neurons;

        Accumulator(std::span<const int16_t, HIDDEN_LAYER_SIZE> biasses);
        Accumulator() = delete;
        ~Accumulator() = default;
        inline void reset(std::span<const int16_t, HIDDEN_LAYER_SIZE> biasses);
    };
    friend bool operator==(const Accumulator &lhs, const Accumulator &rhs);

    constexpr int32_t crelu(const int32_t &input) const;
    constexpr int32_t screlu(const int32_t &input) const;
    ScoreType flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &player,
                                        const std::array<int16_t, HIDDEN_LAYER_SIZE> &adversary) const;

    std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
};

#endif // #ifndef NNUE_H
