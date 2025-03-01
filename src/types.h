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

enum CastlingRights : uint8_t {
    NoCastling,
    WhiteShortCastling,
    WhiteLongCastling = WhiteShortCastling << 1,
    BlackShortCastling = WhiteShortCastling << 2,
    BlackLongCastling = WhiteShortCastling << 3,

    KingSideCastling = WhiteShortCastling | BlackShortCastling,
    QueenSideCastling = WhiteLongCastling | BlackLongCastling,
    WhiteCastling = WhiteShortCastling | WhiteLongCastling,
    BlackCastling = BlackShortCastling | BlackLongCastling,

    AnyCastling = WhiteCastling | BlackCastling
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
constexpr int MaxPly = 256;
constexpr int HalfMovesPerMatch = 2048;
constexpr int MaxSearchDepth = 128;
constexpr int MoveNone = 0;

// clang-format off
constexpr WeightType SEE_values[12] = {100, 300, 300, 500, 1000, 0,
                                       100, 300, 300, 500, 1000, 0};
// clang-format on
constexpr Bitboard FileMasks[8] = {0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
                                   0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080};

constexpr Bitboard RankMasks[8] = {0xff,         0xff00,         0xff0000,         0xff000000,
                                   0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};


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
        captured = Empty;
        fifty_move_ply = 0;
        ply_from_null = 0;
        castling_rights = NoCastling;
        en_passant = NoSquare;
    }
};

#endif // #ifndef GAME_ELEMENTS_H
