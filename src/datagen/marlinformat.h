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

#ifndef MARLINFORMAT_H
#define MARLINFORMAT_H

#include <fstream>

#include "../move.h"
#include "../position.h"
#include "packed_position.h"

class Marlinformat {
  public:
    Marlinformat() : m_pos(Position()) { m_packed_positions.reserve(MAX_MOVES_PER_POS); }

    Marlinformat(const Position &pos) : m_pos(pos) { m_packed_positions.reserve(MAX_MOVES_PER_POS); }

    ~Marlinformat() = default;

    void reset(const Position &pos) {
        m_pos = pos;
        m_packed_positions.clear();
    }

    void push(const Move &move, const ScoreType &score) {
        m_packed_positions.emplace_back(PackedPosition(m_pos, score));
        m_pos.make_move<false>(move);
    }

    void write(std::ofstream &file_out, GameResult result) {
        for (PackedPosition &packed_pos : m_packed_positions) {
            packed_pos.set_result(result);
        }

        file_out.write(reinterpret_cast<const char *>(m_packed_positions.data()),
                       sizeof(PackedPosition) * m_packed_positions.size());
    }

  private:
    Position m_pos;
    std::vector<PackedPosition> m_packed_positions;
};

#endif // #ifndef MARLINFORMAT_H
