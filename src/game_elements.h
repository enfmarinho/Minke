/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_ELEMENTS_H
#define GAME_ELEMENTS_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

using IndexType = int8_t;
using CounterType = int;
using WeightType = int32_t; // TODO change to int16_t
using HashType = uint64_t;
using TimeType = std::chrono::milliseconds::rep;
using TimePoint = std::chrono::steady_clock::time_point;

constexpr WeightType MateScore = 100'000;
constexpr WeightType MaxScore = 200'000;
constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;

using PieceSquareTable = WeightType[BoardHeight][BoardWidth];
using PieceSquareTablePointer = const PieceSquareTable *;

constexpr auto StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MaxMoves = 255;
constexpr int HalfMovesPerMatch = 2048;
constexpr int MaxSearchDepth = 128;

namespace offsets {
constexpr IndexType North = 1;
constexpr IndexType East = 16;
constexpr IndexType South = -North;
constexpr IndexType West = -East;
constexpr IndexType Northeast = North + East;
constexpr IndexType Northwest = North + West;
constexpr IndexType Southeast = South + East;
constexpr IndexType Southwest = South + West;

constexpr IndexType Knight[8] = {East * 2 + North, East * 2 + South, South * 2 + East, South * 2 + West,
                                 West * 2 + South, West * 2 + North, North * 2 + West, North * 2 + East};
constexpr IndexType AllDirections[8] = {East, West, South, North, Southeast, Southwest, Northeast, Northwest};

constexpr IndexType Sliders[3][8] = {
    {Southeast, Southwest, Northeast, Northwest},                          // Bishop
    {North, South, East, West},                                            // Rook
    {East, West, South, North, Southeast, Southwest, Northeast, Northwest} // Queen
};

} // namespace offsets

inline TimeType now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

#define piece_index(piece) static_cast<IndexType>(piece)
enum class Piece : char {
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    None,
};

enum class Player : char {
    White = 1,
    Black = -1,
    None = 0,
};

struct CastlingRights {
    bool king_side;
    bool queen_side;
    CastlingRights() : king_side(false), queen_side(false) {}
};

enum class MoveType : char {
    KingSideCastling,
    QueenSideCastling,
    EnPassant,
    PawnPromotionKnight,
    PawnPromotionBishop,
    PawnPromotionRook,
    PawnPromotionQueen,
    Regular,
    Capture,
};

struct PiecePlacement {
    constexpr static const IndexType padding = 4;
    IndexType file() const { return m_index & 7; }
    IndexType rank() const { return m_index >> padding; }
    IndexType index() const { return m_index; }
    IndexType &index() { return m_index; }
    bool out_of_bounds() const { return m_index & 0x88; }
    PiecePlacement mirrored() const {
        return PiecePlacement(m_index ^ 119); // 119 is (7 << 4) + 7
    }
    PiecePlacement() {};
    PiecePlacement(IndexType index) : m_index(index) {};
    PiecePlacement(const IndexType &file, const IndexType &rank) { m_index = file + (rank << padding); }
    friend bool operator==(const PiecePlacement &lhs, const PiecePlacement &rhs) { return lhs.m_index == rhs.m_index; }

  private:
    IndexType m_index; // file is stored in the 4 least significant bits, while
                       // rank is store in the others 4
};
static_assert(sizeof(PiecePlacement) == 1);

struct Square {
    Piece piece;
    Player player;
    Square() {
        piece = Piece::None;
        player = Player::None;
    }
    Square(Piece piece, Player player) : piece(piece), player(player) {}
    friend bool operator==(const Square &lhs, const Square &rhs) {
        return lhs.piece == rhs.piece && lhs.player == rhs.player;
    }
};
const Square EmptySquare = Square(Piece::None, Player::None);
static_assert(sizeof(Square) == 2);

struct Move {
    PiecePlacement from;
    PiecePlacement to;
    MoveType move_type;
    Move() = default;
    Move(PiecePlacement from, PiecePlacement to, MoveType move_type) : from(from), to(to), move_type(move_type) {}
    std::string get_algebraic_notation() const {
        std::string algebraic_notation;
        algebraic_notation.push_back('a' + from.rank());
        algebraic_notation.push_back('1' + from.file());
        algebraic_notation.push_back('a' + to.rank());
        algebraic_notation.push_back('1' + to.file());
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
    friend bool operator==(const Move &lhs, const Move &rhs) {
        return lhs.move_type == rhs.move_type && lhs.from == rhs.from && lhs.to == rhs.to;
    }
};
const Move MoveNone = Move();
static_assert(sizeof(Move) == 3);

struct PastMove {
    Move movement;
    Square captured;
    IndexType past_en_passant;
    CounterType past_fifty_move_counter;
    CastlingRights past_white_castling_rights;
    CastlingRights past_black_castling_rights;
    PastMove(Move movement, Square captured, IndexType past_en_passant, CounterType past_fifty_move_counter,
             CastlingRights past_white_castling_rights, CastlingRights past_black_castling_rights)
        : movement(movement),
          captured(captured),
          past_en_passant(past_en_passant),
          past_fifty_move_counter(past_fifty_move_counter),
          past_white_castling_rights(past_white_castling_rights),
          past_black_castling_rights(past_black_castling_rights) {}
    PastMove() = default;
};

struct PvList {
    std::array<Move, MaxSearchDepth> m_pv;
    CounterType m_size{0};

    void update(Move new_move, const PvList &list) {
        m_pv[0] = new_move;
        std::copy(list.m_pv.begin(), list.m_pv.begin() + list.m_size, m_pv.begin() + 1);

        m_size = list.m_size + 1;
    }

    void print() const {
        for (int i = 0; i < m_size; ++i) {
            std::cout << m_pv[i].get_algebraic_notation() << ' ';
        }
    }

    PvList &operator=(const PvList &other) {
        std::copy(other.m_pv.begin(), other.m_pv.begin() + other.m_size, m_pv.begin());
        m_size = other.m_size;

        return *this;
    }
};

// clang-format off
// Needed for index conversion, NNUE trainers uses 0 = a8, while my board uses 0 = a1
constexpr IndexType MailboxToStandardNNUE[0x80] = {
    56, 48, 40, 32, 24, 16,  8, 0, 100, 100, 100, 100, 100, 100, 100, 100,
    57, 49, 41, 33, 25, 17,  9, 1, 100, 100, 100, 100, 100, 100, 100, 100,
    58, 50, 42, 34, 26, 18, 10, 2, 100, 100, 100, 100, 100, 100, 100, 100,
    59, 51, 43, 35, 27, 19, 11, 3, 100, 100, 100, 100, 100, 100, 100, 100,
    60, 52, 44, 36, 28, 20, 12, 4, 100, 100, 100, 100, 100, 100, 100, 100,
    61, 53, 45, 37, 29, 21, 13, 5, 100, 100, 100, 100, 100, 100, 100, 100,
    62, 54, 46, 38, 30, 22, 14, 6, 100, 100, 100, 100, 100, 100, 100, 100,
    63, 55, 47, 39, 31, 23, 15, 7, 100, 100, 100, 100, 100, 100, 100, 100,
};
// clang-format on

#endif // #ifndef GAME_ELEMENTS_H
