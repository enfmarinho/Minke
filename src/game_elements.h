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
#include <string>

using IndexType = int8_t;
using CounterType = int;
using WeightType = int32_t; // TODO change to int16_t
using HashType = uint64_t;
using TimeType = std::chrono::milliseconds::rep;
using TimePoint = std::chrono::steady_clock::time_point;

constexpr WeightType ScoreNone = -2000000;
constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;

using PieceSquareTable = WeightType[BoardHeight][BoardWidth];
using PieceSquareTablePointer = const PieceSquareTable *;

constexpr auto StartFEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MaxMoves = 256;
constexpr int HalfMovesPerMatch = 2048;

namespace offsets {
constexpr IndexType North = 1;
constexpr IndexType East = 16;
constexpr IndexType South = -North;
constexpr IndexType West = -East;
constexpr IndexType Northeast = North + East;
constexpr IndexType Northwest = North + West;
constexpr IndexType Southeast = South + East;
constexpr IndexType Southwest = South + West;

constexpr IndexType Knight[8] = {
    East * 2 + North, East * 2 + South, South * 2 + East, South * 2 + West,
    West * 2 + South, West * 2 + North, North * 2 + West, North * 2 + East};
constexpr IndexType AllDirections[8] = {
    East, West, South, North, Southeast, Southwest, Northeast, Northwest};

constexpr IndexType Sliders[3][8] = {
    {Southeast, Southwest, Northeast, Northwest}, // Bishop
    {North, South, East, West},                   // Rook
    {East, West, South, North, Southeast, Southwest, Northeast,
     Northwest} // Queen
};

} // namespace offsets

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
  PiecePlacement() {};
  PiecePlacement(IndexType index) : m_index(index) {};
  PiecePlacement(const IndexType &file, const IndexType &rank) {
    m_index = file + (rank << padding);
  }

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
};
const Square empty_square = Square(Piece::None, Player::None);
static_assert(sizeof(Square) == 2);

struct Move {
  PiecePlacement from;
  PiecePlacement to;
  MoveType move_type;
  Move() = default;
  Move(PiecePlacement from, PiecePlacement to, MoveType move_type)
      : from(from), to(to), move_type(move_type) {}
  std::string get_algebraic_notation() {
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
};
static_assert(sizeof(Move) == 3);

struct PastMove {
  Move movement;
  Square captured;
  IndexType past_en_passant;
  CounterType past_fifty_move_counter;
  CastlingRights past_white_castling_rights;
  CastlingRights past_black_castling_rights;
  PastMove(Move movement, Square captured, IndexType past_en_passant,
           CounterType past_fifty_move_counter,
           CastlingRights past_white_castling_rights,
           CastlingRights past_black_castling_rights)
      : movement(movement), captured(captured),
        past_en_passant(past_en_passant),
        past_fifty_move_counter(past_fifty_move_counter),
        past_white_castling_rights(past_white_castling_rights),
        past_black_castling_rights(past_black_castling_rights) {}
  PastMove() = default;
};

#endif // #ifndef GAME_ELEMENTS_H
