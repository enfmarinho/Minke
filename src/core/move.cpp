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

#include "core/move.h"

#include <cstdint>

#include "core/position.h"
#include "core/types.h"

namespace minke {

constexpr Move::Move(uint16_t raw_packed_move, const Position &pos) {
    const uint16_t from_to = raw_packed_move & 0xFFF;
    const Square to = static_cast<Square>(from_to >> 6);
    const uint16_t move_flag = raw_packed_move >> 14;

    MoveType move_type = REGULAR;
    if (move_flag == 0b01) { // en passant
        move_type = EP;
    } else if (move_flag == 0b10) { // castling
        move_type = CASTLING;
    } else if (move_flag == 0b11) { // promotion
        const uint16_t promo = (raw_packed_move >> 12) & 0b11;
        move_type = static_cast<MoveType>(PAWN_PROMOTION_MASK | promo);
    }

    if (pos.piece_at(to) != EMPTY) { // regular
        move_type = static_cast<MoveType>(move_type | CAPTURE);
    }

    m_bytes = (move_type << 12) | from_to;
}

} // namespace minke
