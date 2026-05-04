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

#ifndef NNUE_H
#define NNUE_H

#include <array>
#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

#include "move.h"
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

struct PieceSquare {
    Piece piece;
    Square sq;

    inline PieceSquare() : piece(EMPTY), sq(NO_SQ) {}
    inline PieceSquare(Piece _piece, Square _sq) : piece(_piece), sq(_sq) {}
    size_t feature_idx(const Color pov) const;
};

struct DirtyPiece {
    PieceSquare add0, add1, sub0, sub1;
    MoveType move_type;
};

// NOTE: NNUE must be initialized using reset().
class NNUE {
  private:
    struct Accumulator;

  public:
    NNUE() = default;
    ~NNUE() = default;

    void pop();
    void push(const DirtyPiece &dp);
    const Accumulator &top() const;

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

    void add(const PieceSquare &ps);
    void sub(const PieceSquare &ps);
    void add_sub(const PieceSquare &add0, const PieceSquare &sub0);
    void add_sub2(const PieceSquare &add0, const PieceSquare &sub0, const PieceSquare &sub1);
    void add2_sub2(const PieceSquare &add0, const PieceSquare &add1, const PieceSquare &sub0, const PieceSquare &sub1);

    constexpr int32_t crelu(const int32_t &input) const;
    constexpr int32_t screlu(const int32_t &input) const;
    ScoreType flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &player,
                                        const std::array<int16_t, HIDDEN_LAYER_SIZE> &adversary) const;

    std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
};

#endif // #ifndef NNUE_H
