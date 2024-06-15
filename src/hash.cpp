/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "hash.h"
#include "game_elements.h"
#include "position.h"

HashType zobrist::hash(const Position &position) {
  HashType key = 0;
  for (IndexType file = 0; file < BoardHeight; ++file)
    for (IndexType rank = 0; rank < BoardWidth; ++rank)
      key ^= ZobristArray[placement_index(file, rank) +
                          piece_start_index(position.consult(file, rank))];

  // En passant possibilities
  if (position.en_passant_rank() != -1)
    key ^= ZobristArray[en_passant_starter_index + position.en_passant_rank()];

  // Castling rights
  CastlingRights white_castling_rights = position.white_castling_rights();
  CastlingRights black_castling_rights = position.black_castling_rights();
  if (white_castling_rights.king_side)
    key ^= ZobristArray[white_king_side_castling_rights_index];
  if (white_castling_rights.queen_side)
    key ^= ZobristArray[white_queen_side_castling_rights_index];
  if (black_castling_rights.king_side)
    key ^= ZobristArray[black_king_side_castling_rights_index];
  if (black_castling_rights.queen_side)
    key ^= ZobristArray[black_queen_side_castling_rights_index];

  // Black turn
  if (position.side_to_move() == Player::Black)
    key ^= ZobristArray[black_turn_index];

  return key;
}

HashType zobrist::rehash(const Position &position) {
  HashType new_key = position.get_hash();
  PastMove last_move = position.last_move();
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
    new_key ^=
        ZobristArray[en_passant_starter_index + last_move.past_en_passant];
  if (position.en_passant_rank() != -1)
    new_key ^=
        ZobristArray[en_passant_starter_index + position.en_passant_rank()];

  // Castling rights
  if (last_move.past_white_castling_rights.king_side !=
      position.white_castling_rights().king_side)
    new_key ^= ZobristArray[white_king_side_castling_rights_index];
  if (last_move.past_white_castling_rights.queen_side !=
      position.white_castling_rights().queen_side)
    new_key ^= ZobristArray[white_queen_side_castling_rights_index];
  if (last_move.past_black_castling_rights.king_side !=
      position.black_castling_rights().king_side)
    new_key ^= ZobristArray[black_king_side_castling_rights_index];
  if (last_move.past_black_castling_rights.queen_side !=
      position.black_castling_rights().queen_side)
    new_key ^= ZobristArray[black_queen_side_castling_rights_index];

  new_key ^= ZobristArray[black_turn_index]; // Change side to move
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
