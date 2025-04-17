/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVE_H
#define MOVE_H

#include <cassert>
#include <cstdint>
#include <string>

#include "types.h"

enum MoveType : char {
    Regular = 0b0000,
    Capture = 0b0100,
    EnPassant = 0b010 | Capture,
    Castling = 0b0011,

    PawnPromotionMask = 0b1000,
    PawnPromotionKnight = PawnPromotionMask | 0b0000,
    PawnPromotionBishop = PawnPromotionMask | 0b0001,
    PawnPromotionRook = PawnPromotionMask | 0b0010,
    PawnPromotionQueen = PawnPromotionMask | 0b011,

    PawnPromotionKnightCapture = PawnPromotionKnight | Capture,
    PawnPromotionBishopCapture = PawnPromotionBishop | Capture,
    PawnPromotionRookCapture = PawnPromotionRook | Capture,
    PawnPromotionQueenCapture = PawnPromotionQueen | Capture,
};

class Move {
  public:
    inline Move() : m_bytes(0){};
    inline Move(int16_t bytes) : m_bytes(bytes){};
    inline Move(Square from, Square to, MoveType move_type) { m_bytes = (move_type << 12) | (to << 6) | (from); }

    inline int from_and_to() const { return m_bytes & 0xFFF; }
    inline Square from() const { return Square(m_bytes & 0x3F); }
    inline Square to() const { return Square((m_bytes >> 6) & 0x3F); }
    inline MoveType type() const { return MoveType((m_bytes >> 12) & 0xF); }
    inline PieceType promotee() const {
        assert(is_promotion());
        return static_cast<PieceType>((type() & 0b0011) + 1);
    }
    inline int16_t internal() const { return m_bytes; }
    std::string get_algebraic_notation() const;

    inline bool is_regular() const { return type() == Regular; }
    inline bool is_castle() const { return type() == Castling; }
    inline bool is_quiet() const { return is_regular() || is_castle(); }
    inline bool is_capture() const { return type() & Capture; }
    inline bool is_promotion() const { return type() & PawnPromotionMask; }
    inline bool is_ep() const { return type() == EnPassant; }
    inline bool is_noisy() const { return is_capture() || is_promotion(); }

    friend bool operator==(const Move& lhs, const Move& rhs);
    friend bool operator==(const Move& lhs, const int& rhs);

  private:
    // Bits are arranged in the following way:
    // 4 bits for move type | 6 bits for target square | 6 bits for origin square
    int16_t m_bytes;
};

struct ScoredMove {
    Move move;
    int score;
};
inline bool operator==(const ScoredMove& lhs, const ScoredMove& rhs) {
    return lhs.move == rhs.move && lhs.score == rhs.score;
}

const Move MoveNone = Move();
const ScoredMove ScoredMoveNone = {MoveNone, 0};

inline bool operator==(const Move& lhs, const Move& rhs) { return lhs.internal() == rhs.internal(); }
inline bool operator==(const Move& lhs, const int& rhs) { return lhs.internal() == rhs; }

#endif // #ifndef MOVE_H
