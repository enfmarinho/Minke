#include "move.h"

#include "utils.h"

Move::Move(Square origin, Square target, MoveType move_type) { m_bytes = (move_type << 12) | (target << 6) | (origin); }

Square Move::from() const { return Square(m_bytes & 0x3F); }

Square Move::to() const { return Square((m_bytes >> 6) & 0x3F); }

MoveType Move::type() const { return MoveType((m_bytes >> 12) & 0xF); }

int16_t Move::internal() const { return m_bytes; }

std::string Move::get_algebraic_notation() const {
    std::string algebraic_notation;
    Square source = from();
    Square target = to();

    algebraic_notation.push_back('a' + get_rank(source));
    algebraic_notation.push_back('1' + get_file(source));
    algebraic_notation.push_back('a' + get_rank(target));
    algebraic_notation.push_back('1' + get_file(target));

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
