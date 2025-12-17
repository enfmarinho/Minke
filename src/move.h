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
    REGULAR = 0b0000,
    CAPTURE = 0b0100,
    EP = 0b010 | CAPTURE,
    CASTLING = 0b0011,

    PAWN_PROMOTION_MASK = 0b1000,
    PAWN_PROMOTION_KNIGHT = PAWN_PROMOTION_MASK | 0b0000,
    PAWN_PROMOTION_BISHOP = PAWN_PROMOTION_MASK | 0b0001,
    PAWN_PROMOTION_ROOK = PAWN_PROMOTION_MASK | 0b0010,
    PAWN_PROMOTION_QUEEN = PAWN_PROMOTION_MASK | 0b011,

    PAWN_PROMOTION_KNIGHT_CAPTURE = PAWN_PROMOTION_KNIGHT | CAPTURE,
    PAWN_PROMOTION_BISHOP_CAPTURE = PAWN_PROMOTION_BISHOP | CAPTURE,
    PAWN_PROMOTION_ROOK_CAPTURE = PAWN_PROMOTION_ROOK | CAPTURE,
    PAWN_PROMOTION_QUEEN_CAPTURE = PAWN_PROMOTION_QUEEN | CAPTURE,
};

class Move {
  public:
    // clang-format off
    inline Move() : m_bytes(0) {};
    inline Move(int16_t bytes) : m_bytes(bytes) {};
    // clang-format on
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
    std::string get_algebraic_notation(const bool chess960, const Bitboard castle_rooks) const;

    inline bool is_regular() const { return type() == REGULAR; }
    inline bool is_castle() const { return type() == CASTLING; }
    inline bool is_quiet() const { return is_regular() || is_castle(); }
    inline bool is_capture() const { return type() & CAPTURE; }
    inline bool is_promotion() const { return type() & PAWN_PROMOTION_MASK; }
    inline bool is_ep() const { return type() == EP; }
    inline bool is_noisy() const { return is_capture() || is_promotion(); }

    friend bool operator==(const Move& lhs, const Move& rhs);
    friend bool operator==(const Move& lhs, const int& rhs);

  private:
    // Bits are arranged in the following way:
    // 4 bits for move type | 6 bits for target square | 6 bits for origin square
    int16_t m_bytes;
};
const Move MOVE_NONE = Move();

struct ScoredMove {
    Move move;
    int score;
};
inline bool operator==(const ScoredMove& lhs, const ScoredMove& rhs) {
    return lhs.move == rhs.move && lhs.score == rhs.score;
}
const ScoredMove SCORED_MOVE_NONE = {MOVE_NONE, 0};

struct MoveList {
    Move moves[MAX_MOVES_PER_POS];
    int size{0};

    void clear() { size = 0; }

    void push(Move move) {
        assert(size < MAX_MOVES_PER_POS);
        moves[size++] = move;
    }
};

struct PieceMove {
    Move move;
    Piece piece;

    PieceMove() { PieceMove(MOVE_NONE, EMPTY); }
    PieceMove(const Move& move, const Piece& piece) : move(move), piece(piece) {}
};
inline bool operator==(const PieceMove& lhs, const PieceMove& rhs) {
    return lhs.move == rhs.move && lhs.piece == rhs.piece;
}
const PieceMove PIECE_MOVE_NONE = {MOVE_NONE, EMPTY};

struct PieceMoveList {
    PieceMove list[MAX_MOVES_PER_POS];
    int size{0};

    void clear() { size = 0; }

    void push(const PieceMove& pt_move) {
        assert(size < MAX_MOVES_PER_POS);
        list[size++] = pt_move;
    }
};

inline bool operator==(const Move& lhs, const Move& rhs) { return lhs.internal() == rhs.internal(); }
inline bool operator==(const Move& lhs, const int& rhs) { return lhs.internal() == rhs; }

#endif // #ifndef MOVE_H
