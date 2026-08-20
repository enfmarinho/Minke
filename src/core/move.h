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

#include <cassert>
#include <cstdint>

#include "core/types.h"
#include "utils/static_vector.h"

class Move {
  public:
    constexpr Move() : m_bytes(0) {}
    constexpr Move(Square from, Square to, MoveType move_type) { m_bytes = (move_type << 12) | (to << 6) | (from); }

    static constexpr Move none() { return Move(); }

    constexpr int from_and_to() const { return m_bytes & 0xFFF; }
    constexpr Square from() const { return Square(m_bytes & 0x3F); }
    constexpr Square to() const { return Square((m_bytes >> 6) & 0x3F); }
    constexpr MoveType type() const { return MoveType((m_bytes >> 12) & 0xF); }
    constexpr PieceType promotee() const {
        assert(is_promotion());
        return static_cast<PieceType>((type() & 0b0011) + 1);
    }

    constexpr bool is_regular() const { return type() == REGULAR; }
    constexpr bool is_castle() const { return type() == CASTLING; }
    constexpr bool is_quiet() const { return is_regular() || is_castle(); }
    constexpr bool is_capture() const { return type() & CAPTURE; }
    constexpr bool is_promotion() const { return type() & PAWN_PROMOTION_MASK; }
    constexpr bool is_ep() const { return type() == EP; }
    constexpr bool is_noisy() const { return is_capture() || is_promotion(); }
    constexpr bool is_none() const { return m_bytes == 0; }
    constexpr explicit operator bool() const { return !is_none(); }

    constexpr bool operator==(const Move&) const = default;

  private:
    // Bits are arranged in the following way:
    // 4 bits for move type | 6 bits for target square | 6 bits for origin square
    uint16_t m_bytes;
};

struct ScoredMove {
    Move move;
    int score;

    static constexpr ScoredMove none() { return {Move::none(), 0}; }

    constexpr bool is_none() const { return move.is_none(); }
    constexpr explicit operator bool() const { return !is_none(); }
};

struct PieceMove {
    Move move;
    Piece piece;

    static constexpr PieceMove none() { return {Move::none(), EMPTY}; }

    constexpr bool is_none() const { return move.is_none(); }
    constexpr explicit operator bool() const { return !is_none(); }
};

using MoveList = StaticVector<Move, MAX_MOVES_PER_POS>;
using PieceMoveList = StaticVector<PieceMove, MAX_MOVES_PER_POS>;
