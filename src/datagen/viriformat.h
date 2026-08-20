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

#include <cstdint>
#include <fstream>

#include "core/move.h"
#include "core/position.h"
#include "datagen/packed_position.h"

class Viriformat {
  public:
    Viriformat();
    Viriformat(const Position &pos);
    ~Viriformat() = default;

    void reset(const Position &pos);
    void push(const Move &move, const ScoreType &score);
    void write(std::ofstream &file_out, GameResult result);

  private:
    struct MoveScore {
        uint16_t packed_move;
        int16_t score;

        MoveScore(uint16_t _packed_move, int16_t _score) : packed_move(_packed_move), score(_score) {}
    };
    static_assert(sizeof(MoveScore) == 4, "MoveScore struct is not 4 bytes");

    PackedPosition m_initial_pos;
    std::vector<MoveScore> m_moves_scores; // move and score for this ply
};
