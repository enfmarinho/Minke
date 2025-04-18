/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move.h"

#include "types.h"
#include "utils.h"

std::string Move::get_algebraic_notation() const {
    std::string algebraic_notation;
    Square source = from();
    Square target = to();

    algebraic_notation.push_back('a' + get_file(source));
    algebraic_notation.push_back('1' + get_rank(source));
    algebraic_notation.push_back('a' + get_file(target));
    algebraic_notation.push_back('1' + get_rank(target));

    int move_type = type() & (~CAPTURE);
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
