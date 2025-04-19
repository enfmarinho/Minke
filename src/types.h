/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_ELEMENTS_H
#define GAME_ELEMENTS_H

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
    DOUBLE_SOUTH = 2 * SOUTH,
};

enum BoundType : char {
    BOUND_EMPTY,
    EXACT,
    LOWER,
    UPPER,
};

using HashType = uint64_t;
using IndexType = int8_t;
using CounterType = int;
using ScoreType = int32_t; // TODO change to int16_t
using TimeType = std::chrono::milliseconds::rep;
using TimePoint = std::chrono::steady_clock::time_point;
using Bitboard = uint64_t;

constexpr int MAX_MOVES_PER_POS = 256;
constexpr int MAX_SEARCH_DEPTH = 256;
constexpr int MAX_PLY = MAX_SEARCH_DEPTH + 100 + 5; // Plus 100 because of fifty move rule and plus 5 just to be safe

constexpr ScoreType MATE_SCORE = 100'000;
constexpr ScoreType MATE_FOUND = MATE_SCORE - MAX_SEARCH_DEPTH;
constexpr ScoreType MAX_SCORE = 200'000;
constexpr CounterType NUMBER_OF_PIECES = 6;
constexpr IndexType BOARD_HEIGHT = 8;
constexpr IndexType BOARD_WIDTH = 8;

constexpr inline auto START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// clang-format off
constexpr ScoreType SEE_VALUES[PIECE_NB] = {100, 300, 300, 500, 1000, 5000,
                                            100, 300, 300, 500, 1000, 5000, 0};
// clang-format on
constexpr Bitboard FILE_MASKS[8] = {0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
                                    0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080};

constexpr Bitboard RANK_MASKS[8] = {0xff,         0xff00,         0xff0000,         0xff000000,
                                    0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};

constexpr Bitboard WHITE_OO_CROSSING_MASK = 0x60;
constexpr Bitboard WHITE_OOO_CROSSING_MASK = 0xe;
constexpr Bitboard BLACK_OO_CROSSING_MASK = 0x6000000000000000;
constexpr Bitboard BLACK_OOO_CROSSING_MASK = 0xe00000000000000;

inline TimeType now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

struct BoardState {
    Piece captured;
    int fifty_move_ply;
    int ply_from_null;
    uint8_t castling_rights;
    Square en_passant;
    void reset() {
        captured = EMPTY;
        fifty_move_ply = 0;
        ply_from_null = 0;
        castling_rights = NO_CASTLING;
        en_passant = NO_SQ;
    }
};

#endif // #ifndef GAME_ELEMENTS_H
