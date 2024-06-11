/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "hashing.h"
#include "game_elements.h"
#include "position.h"

HashType zobrist::hash(const Position &position) {
  HashType key = 0;
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      Square current_square = position.consult(file, rank);
      key ^= ZobristArray[placement_index(file, rank) +
                          piece_start_index(current_square)];
    }
  }
  CastlingRights white_castling_rights = position.white_castling_rights();
  CastlingRights black_castling_rights = position.black_castling_rights();
  if (white_castling_rights.king_side) {
    key ^= ZobristArray[white_king_side_castling_rights_index];
  }
  if (white_castling_rights.queen_side) {
    key ^= ZobristArray[white_queen_side_castling_rights_index];
  }
  if (black_castling_rights.king_side) {
    key ^= ZobristArray[black_king_side_castling_rights_index];
  }
  if (black_castling_rights.queen_side) {
    key ^= ZobristArray[black_queen_side_castling_rights_index];
  }
  if (position.en_passant_rank() != -1) {
    key ^= ZobristArray[en_passant_starter_index + position.en_passant_rank()];
  }
  if (position.side_to_move() == Player::Black) {
    key ^= ZobristArray[black_turn_index];
  }
  return key;
}

HashType zobrist::rehash(const Position &position) {
  HashType new_hash = position.get_hash();
  PastMove last_move = position.last_move();
  IndexType piece_index =
      piece_start_index(position.consult(last_move.movement.to));
  IndexType destination_index = placement_index(last_move.movement.to);

  new_hash ^= ZobristArray[piece_index + destination_index];
  new_hash ^=
      ZobristArray[piece_index + placement_index(last_move.movement.from)];
  if (last_move.captured.piece != Piece::None) {
    new_hash ^=
        ZobristArray[piece_start_index(last_move.captured) + destination_index];
  }
  if (last_move.past_en_passant != -1) {
    new_hash ^=
        ZobristArray[en_passant_starter_index + last_move.past_en_passant];
  }
  if (position.en_passant_rank() != -1) {
    new_hash ^=
        ZobristArray[en_passant_starter_index + position.en_passant_rank()];
  }
  if (last_move.past_white_castling_rights.king_side !=
      position.white_castling_rights().king_side) {
    new_hash ^= ZobristArray[white_king_side_castling_rights_index];
  }
  if (last_move.past_white_castling_rights.queen_side !=
      position.white_castling_rights().queen_side) {
    new_hash ^= ZobristArray[white_queen_side_castling_rights_index];
  }
  if (last_move.past_black_castling_rights.king_side !=
      position.black_castling_rights().king_side) {
    new_hash ^= ZobristArray[black_king_side_castling_rights_index];
  }
  if (last_move.past_black_castling_rights.queen_side !=
      position.black_castling_rights().queen_side) {
    new_hash ^= ZobristArray[black_queen_side_castling_rights_index];
  }

  new_hash ^= ZobristArray[black_turn_index];
  return new_hash;
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
