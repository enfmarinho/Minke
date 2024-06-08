/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_ELEMENTS_HPP
#define GAME_ELEMENTS_HPP

#include <chrono>
#include <cstdint>
#include <string>

using IndexType = int8_t;
using CounterType = int;
using WeightType = int32_t; // TODO change to int16_t
using HashType = uint64_t;
using TimePoint = std::chrono::milliseconds::rep;

constexpr WeightType ScoreNone = -2000000;
constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;

// TODO change this to 0x80 boards to facilitate indexing.
using PieceSquareTable = WeightType[BoardHeight][BoardWidth];
using PieceSquareTablePointer = const PieceSquareTable *;

constexpr auto StartFEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MaxMoves = 256;

namespace directions {
constexpr IndexType North = 16;
constexpr IndexType South = -16;
constexpr IndexType East = 1;
constexpr IndexType West = -1;
constexpr IndexType Northeast = 17;
constexpr IndexType Southeast = -15;
constexpr IndexType Northwest = 15;
constexpr IndexType Southwest = -17;
}; // namespace directions
namespace move_offsets {
constexpr IndexType Knight[8] = {directions::East * 2 + directions::North,
                                 directions::East * 2 + directions::South,
                                 directions::South * 2 + directions::East,
                                 directions::South * 2 + directions::West,
                                 directions::West * 2 + directions::South,
                                 directions::West * 2 + directions::North,
                                 directions::North * 2 + directions::West,
                                 directions::North * 2 + directions::East};
constexpr IndexType AllDirections[8] = {
    directions::East,      directions::West,      directions::South,
    directions::North,     directions::Southeast, directions::Southwest,
    directions::Northeast, directions::Northwest};

constexpr IndexType Sliders[3][8] = {
    {directions::Southeast, directions::Southwest, directions::Northeast,
     directions::Northwest}, // Bishop
    {directions::North, directions::South, directions::East,
     directions::West}, // Rook
    {directions::East, directions::West, directions::South, directions::North,
     directions::Southeast, directions::Southwest, directions::Northeast,
     directions::Northwest} // Queen
};

} // namespace move_offsets

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
  PiecePlacement(){};
  PiecePlacement(IndexType index) : m_index(index){};
  PiecePlacement(const IndexType &file, const IndexType &rank) {
    m_index = file + (rank << padding);
  }

private:
  IndexType m_index; // file is stored in the 4 least significant bits, while
                     // rank is store in the others 4
};

struct Square {
  Piece piece;
  Player player;
  Square() {
    piece = Piece::None;
    player = Player::None;
  }
  Square(Piece piece, Player player) : piece(piece), player(player) {}
};

struct Movement {
  PiecePlacement from;
  PiecePlacement to;
  MoveType move_type;
  Movement() = default;
  Movement(PiecePlacement from, PiecePlacement to, MoveType move_type)
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

struct PastMovement {
  Movement movement;
  Square captured;
  IndexType past_en_passant;
  CounterType past_fifty_move_counter;
  CastlingRights past_white_castling_rights;
  CastlingRights past_black_castling_rights;
  PastMovement(Movement movement, Square captured, IndexType past_en_passant,
               CounterType past_fifty_move_counter,
               CastlingRights past_white_castling_rights,
               CastlingRights past_black_castling_rights)
      : movement(movement), captured(captured),
        past_en_passant(past_en_passant),
        past_fifty_move_counter(past_fifty_move_counter),
        past_white_castling_rights(past_white_castling_rights),
        past_black_castling_rights(past_black_castling_rights) {}
};

#endif // #ifndef GAME_ELEMENTS_HPP
