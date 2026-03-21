/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef NNUE_H
#define NNUE_H

#include <cassert>

#include "../types.h"
#include "finny_table.h"

class Position;

class NNUE {
  public:
    NNUE() { finny_table.reset(); }
    ~NNUE() = default;

    void reset();
    ScoreType eval(const Position &stm);
    ScoreType flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &player,
                                        const std::array<int16_t, HIDDEN_LAYER_SIZE> &adversary) const;

  private:
    FinnyTable finny_table;
};

#endif // #ifndef NNUE_H
