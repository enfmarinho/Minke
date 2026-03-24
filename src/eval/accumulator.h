/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../move.h"
#include "../utils.h"
#include "arch.h"

// Auxiliary functions
/// Calculate the input layer feature index
size_t feature_idx(const Piece piece, const Square sq, const Color stm, const size_t king_bucket, const bool flip);
/// Check whether king has crossed half of the board, i.e. if the board should be flipped
inline bool should_flip(const Square king_pov_sq) { return get_file(king_pov_sq) > 3; }

class Position;

struct SquarePiece {
    Square sq;
    Piece piece;

    inline SquarePiece() {
        sq = NO_SQ;
        piece = EMPTY;
    }
    inline SquarePiece(Square _sq, Piece _piece) : sq(_sq), piece(_piece) {}
};

struct DirtyPiece {
    SquarePiece add0, add1, sub0, sub1;
    MoveType move_type;
};

struct alignas(64) PovAccumulator {
    std::array<int16_t, HIDDEN_LAYER_SIZE> neurons;

    PovAccumulator() { reset(); }
    ~PovAccumulator() = default;

    inline void reset() { memcpy(neurons.data(), network.hidden_bias.data(), sizeof(network.hidden_bias)); }

    void self_add(const size_t feature_idx);
    void self_sub(const size_t feature_idx);
    void self_add_sub(const size_t add0, const size_t sub0);

    void add(const int16_t *input, const size_t feature_idx);
    void sub(const int16_t *input, const size_t feature_idx);

    void add_sub(const int16_t *input, const size_t add0, const size_t sub0);
    void add_sub2(const int16_t *input, const size_t add0, const size_t sub0, const size_t sub1);
    void add2_sub2(const int16_t *input, const size_t add0, const size_t add1, const size_t sub0, const size_t sub1);
};

class alignas(64) Accumulator {
  public:
    Accumulator() = delete;
    Accumulator(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq);
    ~Accumulator() = default;

    inline const PovAccumulator &neurons(const Color &side) { return m_pov_accumulators[side]; }
    inline bool updated(const Color &side) { return m_updated[side]; }
    void update(const Color side, const Position &pos, const Accumulator &prev_acc);

    void set_king_sqs(const Square white_king_sq, const Square black_king_sq);
    void set_dirty_piece(const DirtyPiece dp);

    void reset();
    bool needs_refresh(const Color side, const Square new_king_sq);
    void refresh(const Color side, const PovAccumulator &finny_table_neurons);

  private:
    PovAccumulator m_pov_accumulators[2];
    bool m_updated[2];
    Square m_king_sqs[2];
    DirtyPiece m_dirty_piece;
};

#endif // ACCUMULATOR_H
