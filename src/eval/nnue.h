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

#include <cassert>
#include <cstdint>

#include "../types.h"
#include "accumulator.h"
#include "arch.h"

class Position;

// NOTE: must be initialized using init().
class NNUE {
  public:
    NNUE() = default;
    ~NNUE() = default;

    void init(const Position &position);

    void apply_move(const DirtyPiece &dp);
    void unapply_move();

    ScoreType eval(const Position &pos);

  private:
    void update();
    void pov_update(const Color pov);

    ScoreType flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &stm,
                                        const std::array<int16_t, HIDDEN_LAYER_SIZE> &ntm) const;

    std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
};

#endif // #ifndef NNUE_H
