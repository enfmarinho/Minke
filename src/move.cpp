/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move.h"

#include "types.h"
#include "utils.h"

Move::Move(Square origin, Square target, MoveType move_type) { m_bytes = (move_type << 12) | (target << 6) | (origin); }

Square Move::from() const { return Square(m_bytes & 0x3F); }

Square Move::to() const { return Square((m_bytes >> 6) & 0x3F); }

MoveType Move::type() const { return MoveType((m_bytes >> 12) & 0xF); }

PieceType Move::promotee() const {
    assert(is_promotion());
    return static_cast<PieceType>((type() & 0b0011) + 1);
}

int16_t Move::internal() const { return m_bytes; }

std::string Move::get_algebraic_notation() const {
    std::string algebraic_notation;
    Square source = from();
    Square target = to();

    algebraic_notation.push_back('a' + get_file(source));
    algebraic_notation.push_back('1' + get_rank(source));
    algebraic_notation.push_back('a' + get_file(target));
    algebraic_notation.push_back('1' + get_rank(target));

    int move_type = type() & (!Capture);
    if (move_type == MoveType::PawnPromotionQueen) {
        algebraic_notation.push_back('q');
    } else if (move_type == MoveType::PawnPromotionKnight) {
        algebraic_notation.push_back('n');
    } else if (move_type == MoveType::PawnPromotionRook) {
        algebraic_notation.push_back('r');
    } else if (move_type == MoveType::PawnPromotionBishop) {
        algebraic_notation.push_back('b');
    }
    return algebraic_notation;
}

bool operator==(const Move& lhs, const Move& rhs) { return lhs.internal() == rhs.internal(); }

bool operator==(const Move& lhs, const int& rhs) { return lhs.internal() == rhs; }
