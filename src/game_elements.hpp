/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_ELEMENTS_HPP
#define GAME_ELEMENTS_HPP

#include <cstdint>
#include <vector>

using IndexType = int; // TODO change to int8_t
using CounterType = int;
using WeightType = int32_t; // TODO change to int16_t
using HashType = uint64_t;

constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;
constexpr IndexType NumberOfFiles = 12;
constexpr IndexType NumberOfRanks = 10;
constexpr IndexType RankOffset = NumberOfRanks - BoardWidth;
constexpr IndexType FileOffset = NumberOfFiles - BoardHeight;

using PieceSquareTable = WeightType[BoardHeight][BoardWidth];
using PieceSquareTablePointer = const PieceSquareTable *;

constexpr auto StartFEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MaxMoves = 256;

enum class Piece : char {
  Pawn = 0,
  Knight,
  Bishop,
  Rook,
  Queen,
  King,
  None,
  OutOfBounds,
};

enum class Player : char {
  White = 1,
  Black = -1,
  None = 0,
};

struct CastlingRights {
  bool king_side;
  bool queen_side;
  CastlingRights() : king_side(true), queen_side(true) {}
};

enum class MoveType : char {
  KingSideCastling,
  QueenSideCastling,
  EnPassant,
  PawnPromotion,
  Regular,
};

struct PiecePlacement {
  constexpr static const int8_t padding = 4;
  IndexType file() const { return m_data & padding; }
  IndexType rank() const { return m_data >> padding; }
  PiecePlacement() {}
  PiecePlacement(const IndexType &file, const IndexType &rank) {
    m_data = file + (rank << padding);
  }

private:
  uint8_t m_data; // file is stored in the 4 least significant bits, while rank
                  // is store in the others 4
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
  Movement(PiecePlacement from, PiecePlacement to, Square captured,
           MoveType move_type)
      : from(from), to(to), move_type(move_type) {}
};
using MovementList = std::vector<Movement>;

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
