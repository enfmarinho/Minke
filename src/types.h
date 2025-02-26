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
    NoSquare
};
// clang-format on

constexpr int ColorOffset = 6;

enum Color : int {
    White,
    Black,
};

enum PieceType : int {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    None
};

enum Piece : int {
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,

    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,

    Empty
};

enum CastlingRights : int {
    WhiteShortCastle = 1,
    WhiteLongCastle = 2,
    BlackShortCastle = 4,
    BlackLongCastle = 8
};

enum Direction : int {
    North = 8,
    South = -8,
    West = -1,
    East = 1,
    NorthEast = North + East,
    NorthWest = North + West,
    SouthEast = South + East,
    SouthWest = South + West,

    DoubleNorth = 2 * North,
    DoubleSouth = 2 * South,
};

using HashType = uint64_t;
using IndexType = int8_t;
using CounterType = int;
using WeightType = int32_t; // TODO change to int16_t
using TimeType = std::chrono::milliseconds::rep;
using TimePoint = std::chrono::steady_clock::time_point;
using Bitboard = uint64_t;

constexpr WeightType MateScore = 100'000;
constexpr WeightType MaxScore = 200'000;
constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;

constexpr inline auto StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MaxMoves = 256;
constexpr int HalfMovesPerMatch = 2048;
constexpr int MaxSearchDepth = 128;
constexpr int MoveNone = 0;

inline TimeType now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

struct BoardState {
    PieceType captured;
    int fifty_move_clock;
    int ply_after_reset;
    uint8_t castle_rights;
    Square en_passant;
};

#endif // #ifndef GAME_ELEMENTS_H
