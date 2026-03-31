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

#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "../types.h"
#include "pov_accumulator.h"

class Position;

class alignas(64) Accumulator {
  public:
    Accumulator() = delete;
    Accumulator(const Position &pos);
    Accumulator(const DirtyPiece &dp);
    ~Accumulator() = default;

    void reset(const Position &pos);
    void update(const Color pov, const PovAccumulator &prev_pov_acc);
    inline bool updated(const Color pov) { return m_updated[pov]; }
    inline const PovAccumulator &pov(const Color pov) const { return m_pov_accumulators[pov]; }

    friend bool operator==(const Accumulator &lhs, const Accumulator &rhs);

  private:
    void reset_pov(const Color pov, const Position &pos);

    alignas(64) PovAccumulator m_pov_accumulators[2];
    bool m_updated[2];
    DirtyPiece m_dirty_piece;
};

#endif // #ifndef ACCUMULATOR_H
