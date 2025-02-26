/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVE_H
#define MOVE_H

#include <cstdint>
#include <string>

#include "types.h"

enum MoveType : char {
    Regular = 0b0000,
    Capture = 0b0001,
    EnPassant = 0b0011,
    KingSideCastling = 0b0100,
    QueenSideCastling = 0b0110,

    PawnPromotionMask = 0b1000,
    PawnPromotionKnight = PawnPromotionMask | 0b0000,
    PawnPromotionBishop = PawnPromotionMask | 0b0100,
    PawnPromotionRook = PawnPromotionMask | 0b0010,
    PawnPromotionQueen = PawnPromotionMask | 0b0110,

    PawnPromotionKnightCapture = PawnPromotionKnight | Capture,
    PawnPromotionBishopCapture = PawnPromotionBishop | Capture,
    PawnPromotionRookCapture = PawnPromotionRook | Capture,
    PawnPromotionQueenCapture = PawnPromotionQueen | Capture,
};

class Move {
  public:
    Move() : m_bytes() {};
    Move(int16_t bytes) : m_bytes(bytes) {};
    Move(Square from, Square to, MoveType move_type);

    Square from() const;
    Square to() const;
    MoveType type() const;
    int16_t internal() const;
    std::string get_algebraic_notation() const;

    friend bool operator==(const Move& lhs, const Move& rhs);
    friend bool operator==(const Move& lhs, const int& rhs);

  private:
    // Bits are arranged in the following way:
    // 4 bits for move type | 6 bits for target square | 6 bits for origin square
    int16_t m_bytes;
};

bool operator==(const Move& lhs, const Move& rhs);
bool operator==(const Move& lhs, const int& rhs);

#endif // #ifndef MOVE_H
