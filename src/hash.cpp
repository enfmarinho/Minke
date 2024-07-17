/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "hash.h"
#include "game_elements.h"
#include "position.h"
#include <array>
#include <limits>
#include <random>

// Random array of unsigned 64 bit integers for performing Zobrist Hashing.
//
// The indexing for the pieces is as follows: file * BoardWidth + rank +
// piece_index * BoardWidth * BoardHeight. The piece_index is the value of the
// enum Piece for the respective piece, but adding "NumberOfPieces" (which is
// 6) if the piece is black. With that, the 768 first values are used for the
// pieces, the other 13 are used for position characteristics, like
// castling rights, a possible en passant and the side to move.
const static std::array<HashType, 781> ZobristArray = [] {
  std::mt19937_64 gen(0);
  std::uniform_int_distribution<HashType> dist(
      1, std::numeric_limits<HashType>::max());

  std::array<uint64_t, 781> table;
  for (HashType &value : table)
    value = dist(gen);

  return table;
}();

HashType zobrist::hash(const Position &position) {
  HashType key = 0;
  for (IndexType file = 0; file < BoardHeight; ++file)
    for (IndexType rank = 0; rank < BoardWidth; ++rank)
      key ^= ZobristArray[placement_index(file, rank) +
                          piece_start_index(position.consult(file, rank))];

  // En passant possibilities
  if (position.en_passant_rank() != -1)
    key ^= ZobristArray[EnPassantStarterIndex + position.en_passant_rank()];

  // Castling rights
  CastlingRights white_castling_rights = position.white_castling_rights();
  CastlingRights black_castling_rights = position.black_castling_rights();
  if (white_castling_rights.king_side)
    key ^= ZobristArray[WhiteKingSideCastlingRightsIndex];
  if (white_castling_rights.queen_side)
    key ^= ZobristArray[WhiteQueenSideCastlingRightsIndex];
  if (black_castling_rights.king_side)
    key ^= ZobristArray[BlackKingSideCastlingRightsIndex];
  if (black_castling_rights.queen_side)
    key ^= ZobristArray[BlackQueenSideCastlingRightsIndex];

  // Black turn
  if (position.side_to_move() == Player::Black)
    key ^= ZobristArray[BlackTurnIndex];

  return key;
}

HashType zobrist::rehash(const Position &position, const PastMove &last_move) {
  HashType new_key = position.get_hash();
  IndexType piece_index =
      piece_start_index(position.consult(last_move.movement.to));
  IndexType destination_index = placement_index(last_move.movement.to);

  // Moved piece
  new_key ^= ZobristArray[piece_index + destination_index];
  if (last_move.movement.move_type == MoveType::PawnPromotionQueen ||
      last_move.movement.move_type == MoveType::PawnPromotionRook ||
      last_move.movement.move_type == MoveType::PawnPromotionBishop ||
      last_move.movement.move_type == MoveType::PawnPromotionKnight) {
    new_key ^=
        ZobristArray[piece_start_index(Square(
                         Piece::Pawn,
                         (position.consult(last_move.movement.to).player))) +
                     placement_index(last_move.movement.from)];
  } else {
    new_key ^=
        ZobristArray[piece_index + placement_index(last_move.movement.from)];
  }

  if (last_move.captured.piece != Piece::None) {               // Capture
    if (last_move.movement.move_type == MoveType::EnPassant) { // En passant
      IndexType offset = last_move.captured.player == Player::White
                             ? offsets::North
                             : offsets::South;
      new_key ^=
          ZobristArray[piece_start_index(last_move.captured) +
                       placement_index(last_move.movement.to.index() + offset)];
    } else
      new_key ^= ZobristArray[piece_start_index(last_move.captured) +
                              destination_index];
  } else if (last_move.movement.move_type == MoveType::KingSideCastling) {
    Player player = position.consult(last_move.movement.to).player;
    IndexType first_file_player_perpective = player == Player::White ? 0 : 7;
    IndexType rook_index = piece_start_index(Square(Piece::Rook, player));
    new_key ^= ZobristArray[rook_index +
                            placement_index(first_file_player_perpective, 7)];
    new_key ^= ZobristArray[rook_index +
                            placement_index(first_file_player_perpective, 5)];
  } else if (last_move.movement.move_type == MoveType::QueenSideCastling) {
    Player player = position.consult(last_move.movement.to).player;
    IndexType first_file_player_perpective = player == Player::White ? 0 : 7;
    IndexType rook_index = piece_start_index(Square(Piece::Rook, player));
    new_key ^= ZobristArray[rook_index +
                            placement_index(first_file_player_perpective, 0)];
    new_key ^= ZobristArray[rook_index +
                            placement_index(first_file_player_perpective, 3)];
  }

  // En passant possibilities
  if (last_move.past_en_passant != -1)
    new_key ^= ZobristArray[EnPassantStarterIndex + last_move.past_en_passant];
  if (position.en_passant_rank() != -1)
    new_key ^= ZobristArray[EnPassantStarterIndex + position.en_passant_rank()];

  // Castling rights
  if (last_move.past_white_castling_rights.king_side !=
      position.white_castling_rights().king_side)
    new_key ^= ZobristArray[WhiteKingSideCastlingRightsIndex];
  if (last_move.past_white_castling_rights.queen_side !=
      position.white_castling_rights().queen_side)
    new_key ^= ZobristArray[WhiteQueenSideCastlingRightsIndex];
  if (last_move.past_black_castling_rights.king_side !=
      position.black_castling_rights().king_side)
    new_key ^= ZobristArray[BlackKingSideCastlingRightsIndex];
  if (last_move.past_black_castling_rights.queen_side !=
      position.black_castling_rights().queen_side)
    new_key ^= ZobristArray[BlackQueenSideCastlingRightsIndex];

  new_key ^= ZobristArray[BlackTurnIndex]; // Change side to move
  return new_key;
}

int zobrist::piece_start_index(const Square &piece_square) {
  return static_cast<int>(BoardWidth * BoardHeight) *
         (static_cast<int>(piece_square.piece) +
          (piece_square.player == Player::White ? 0 : NumberOfPieces));
}

IndexType zobrist::placement_index(const PiecePlacement &piece_placement) {
  return piece_placement.file() * BoardWidth + piece_placement.rank();
}

IndexType zobrist::placement_index(const IndexType &file,
                                   const IndexType &rank) {
  return file * BoardWidth + rank;
}
