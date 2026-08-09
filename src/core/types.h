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

#include <chrono>
#include <cstdint>

// clang-format off
enum Square : int {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    NO_SQ
};
// clang-format on

constexpr int COLOR_OFFSET = 6;

enum Color : int {
    WHITE,
    BLACK,
    COLOR_NB
};

enum PieceType : int {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NONE,
    PIECE_TYPE_NB
};

enum Piece : int {
    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,

    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,

    EMPTY,
    PIECE_NB
};

enum CastlingRights : uint8_t {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE_CASTLING = WHITE_OO | BLACK_OO,
    QUEEN_SIDE_CASTLING = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,

    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

enum Direction : int {
    NORTH = 8,
    SOUTH = -8,
    WEST = -1,
    EAST = 1,

    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,

    DOUBLE_NORTH = 2 * NORTH,
    DOUBLE_NORTH_EAST = 2 * NORTH + EAST,
    DOUBLE_NORTH_WEST = 2 * NORTH + WEST,

    DOUBLE_SOUTH = 2 * SOUTH,
    DOUBLE_SOUTH_EAST = 2 * SOUTH + EAST,
    DOUBLE_SOUTH_WEST = 2 * SOUTH + WEST,

    DOUBLE_EAST_NORTH = 2 * EAST + NORTH,
    DOUBLE_WEST_NORTH = 2 * WEST + NORTH,

    DOUBLE_EAST_SOUTH = 2 * EAST + SOUTH,
    DOUBLE_WEST_SOUTH = 2 * WEST + SOUTH,
};

enum BoundType : char {
    BOUND_EMPTY,
    EXACT,
    LOWER,
    UPPER,
};

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

using HashType = uint64_t;
using KeyType = uint16_t;
using IndexType = uint8_t;
using CounterType = int;
using HistoryType = int16_t;
using ScoreType = int16_t;
using TimeType = std::chrono::milliseconds::rep;
using TimePoint = std::chrono::steady_clock::time_point;

constexpr int MAX_MOVES_PER_POS = 256;
constexpr int MAX_SEARCH_DEPTH = 256;
constexpr int MAX_PLY = MAX_SEARCH_DEPTH + 100 + 5; // Plus 100 because of fifty move rule and plus 5 just to be safe

constexpr ScoreType MATE_SCORE = 32000;
constexpr ScoreType MATE_FOUND = MATE_SCORE - MAX_SEARCH_DEPTH;
constexpr ScoreType MAX_SCORE = 32500;
constexpr ScoreType SCORE_NONE = MAX_SCORE + 10;
constexpr CounterType NUMBER_OF_PIECES = 6;
constexpr IndexType BOARD_HEIGHT = 8;
constexpr IndexType BOARD_WIDTH = 8;

constexpr inline auto START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

inline TimeType now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

struct PieceSquare {
    Piece piece;
    Square sq;

    inline PieceSquare() : piece(EMPTY), sq(NO_SQ) {}
    inline PieceSquare(Piece _piece, Square _sq) : piece(_piece), sq(_sq) {}
};

struct DirtyPiece {
    PieceSquare add0, add1, sub0, sub1;
    MoveType move_type;
};
