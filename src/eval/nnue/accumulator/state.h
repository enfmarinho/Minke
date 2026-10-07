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

#pragma once

#include "core/types.h"
#include "eval/nnue/accumulator/perspective.h"

namespace minke::nnue::accumulator {

class alignas(64) State {
  public:
    State() = delete;
    State(const Perspective &white_perspective_acc, const Perspective &black_perspective_acc, Square white_king_sq,
          Square black_king_sq);
    State(DirtyPiece dp, Square white_king_sq, Square black_king_sq);
    ~State() = default;

    void update(const Perspective &prev_perspective_acc, Color pov);
    inline bool updated(Color pov) const { return m_updated[pov]; }

    bool needs_refresh(Color side, Square new_king_sq) const;
    void refresh(const Perspective &finny_table_neurons, Color side);

    inline const Perspective &pov(Color pov) const { return m_perspective_accs[pov]; }

    friend bool operator==(const State &lhs, const State &rhs);

  private:
    void init(DirtyPiece dp, Square white_king_sq, Square black_king_sq);

    alignas(64) Perspective m_perspective_accs[2];
    bool m_updated[2];
    Square m_king_sqs[2];
    DirtyPiece m_dirty_piece;
};

} // namespace minke::nnue::accumulator
