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

#include "move.h"

#include "types.h"
#include "utils.h"

std::string Move::get_algebraic_notation(const bool chess960, const Bitboard castle_rooks) const {
    std::string algebraic_notation;
    Square source = from();
    Square target = to();
    int move_type = type() & (~CAPTURE);

    if (chess960 && move_type == CASTLING) {
        Bitboard bb = castle_rooks & RANK_MASKS[get_rank(source)];
        if (source > target)
            target = lsb(bb);
        else
            target = msb(bb);
    }
    algebraic_notation.push_back('a' + get_file(source));
    algebraic_notation.push_back('1' + get_rank(source));
    algebraic_notation.push_back('a' + get_file(target));
    algebraic_notation.push_back('1' + get_rank(target));

    if (move_type == MoveType::PAWN_PROMOTION_QUEEN)
        algebraic_notation.push_back('q');
    else if (move_type == MoveType::PAWN_PROMOTION_KNIGHT)
        algebraic_notation.push_back('n');
    else if (move_type == MoveType::PAWN_PROMOTION_ROOK)
        algebraic_notation.push_back('r');
    else if (move_type == MoveType::PAWN_PROMOTION_BISHOP)
        algebraic_notation.push_back('b');

    return algebraic_notation;
}
