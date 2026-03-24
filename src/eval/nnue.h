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

#include "../types.h"
#include "accumulator.h"
#include "arch.h"
#include "finny_table.h"

class Position;

class NNUE {
  public:
    NNUE();
    ~NNUE() = default;

    void init(const Position &pos);

    void apply_move(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq);
    void unapply_move();

    ScoreType eval(const Position &stm);

  private:
    void update(const Position &pos);
    void side_relative_update(const Color &side, const Position &pos);

    ScoreType flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &player,
                                        const std::array<int16_t, HIDDEN_LAYER_SIZE> &adversary) const;

    std::vector<Accumulator> m_acc_stack;
    FinnyTable m_finny_table;
};

#endif // #ifndef NNUE_H
