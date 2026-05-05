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
#include <vector>

#include "../types.h"
#include "nnue/accumulator.h"
#include "nnue/pov_accumulator.h"

class Position;

// NOTE: NNUE must be initialized using refresh().
class NNUE {
  public:
    NNUE() = default;
    ~NNUE() = default;

    void refresh(const Position &position);

    void pop();
    void push(const DirtyPiece &dp);

    ScoreType eval(const Color &stm);

  private:
    void update();
    void update_pov(const Color &pov);
    ScoreType flatten_screlu_and_affine(const PovAccumulator &player, const PovAccumulator &adversary) const;

    std::vector<Accumulator> m_accumulators; //!< Stack with accumulators
};

#endif // #ifndef NNUE_H
