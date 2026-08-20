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
#include <cstring>

#include "core/position.h"
#include "core/types.h"

// White perspective
enum GameResult : uint8_t {
    LOSS,
    DRAW,
    WIN,
    NO_RESULT
};

class __attribute__((packed)) PackedPosition {
  public:
    PackedPosition(const Position &position, ScoreType score);

    void set_result(uint8_t result);

  private:
    uint64_t m_occupancy;
    uint8_t m_pieces[16];
    uint8_t m_stm_ep_sq;
    uint8_t m_half_move_counter;
    uint16_t m_game_clock;
    int16_t m_score;
    uint8_t m_result;
    uint8_t m_padding;
};
static_assert(sizeof(PackedPosition) == 32, "PackedPosition struct is not 32 bytes");
