/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_ELEMENTS_H
#define GAME_ELEMENTS_H

#include <cstdint>
#include <vector>

using i32 = int32_t;
using IndexType = int;
using CounterType = int;
using WeightType = i32;

constexpr CounterType NumberOfPieces = 6;
constexpr IndexType BoardHeight = 8;
constexpr IndexType BoardWidth = 8;
constexpr IndexType NumberOfFiles = 12;
constexpr IndexType NumberOfRanks = 10;
constexpr IndexType RankOffset = NumberOfRanks - BoardWidth;
constexpr IndexType FileOffset = NumberOfFiles - BoardHeight;

using PieceSquareTable = WeightType[BoardHeight][BoardWidth];
using PieceSquareTablePointer = const PieceSquareTable *;

enum class Piece {
  Pawn = 0,
  Bishop,
  Knight,
  Rook,
  Queen,
  King,
  None,
  OutOfBounds,
};

enum class Player {
  White = 1,
  Black = -1,
  None,
};

struct CastlingRights {
  bool king_side;
  bool queen_side;
  CastlingRights() : king_side(true), queen_side(true) {}
};

enum class MoveType {
  KingSideCastling,
  QueenSideCastling,
  EnPassant,
  PawnPromotion,
  Regular,
};

struct Position {
  IndexType file;
  IndexType rank;
  Position() = delete;
  Position(IndexType file, IndexType rank) : file(file), rank(rank) {}
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
  Position from;
  Position to;
  MoveType move_type;
  Movement(Position from, Position to, Square captured, MoveType move_type)
      : from(from), to(to), move_type(move_type) {}
};
using MovementList = std::vector<Movement>;

struct PastMovement {
  Movement movement;
  Square captured;
  Position past_en_passant;
  CounterType past_fifty_move_counter;
  PastMovement(Movement movement, Square captured, int fifty_move_counter,
               Position en_passant)
      : movement(movement), captured(captured), past_en_passant(en_passant),
        past_fifty_move_counter(fifty_move_counter) {}
};

#endif // #ifndef GAME_ELEMENTS_H
