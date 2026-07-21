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

#include "../../types.h"
#include "pov_accumulator.h"

class Position;

class alignas(64) Accumulator {
  public:
    Accumulator() = delete;
    Accumulator(const Square white_king_sq, const Square black_king_sq, const PovAccumulator &white_pov_acc,
                const PovAccumulator &black_pov_acc);
    Accumulator(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq);
    ~Accumulator() = default;

    void update(const Color pov, const PovAccumulator &prev_pov_acc);
    inline bool updated(const Color pov) const { return m_updated[pov]; }

    bool needs_refresh(const Color side, const Square new_king_sq) const;
    void refresh(const Color side, const PovAccumulator &finny_table_neurons);

    inline const PovAccumulator &pov(const Color pov) const { return m_pov_accumulators[pov]; }

    friend bool operator==(const Accumulator &lhs, const Accumulator &rhs);

  private:
    void init(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq);

    alignas(64) PovAccumulator m_pov_accumulators[2];
    bool m_updated[2];
    Square m_king_sqs[2];
    DirtyPiece m_dirty_piece;
};

#endif // #ifndef ACCUMULATOR_H
