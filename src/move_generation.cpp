/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move_generation.hpp"

MoveList::MoveList(const Position &position) {
  m_end = m_movement_list;
  if (double_check()) {
    legal_king_moves(position.side_to_move() == Player::White
                         ? position.white_king_position()
                         : position.black_king_position());
    return;
  }
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement current_square = PiecePlacement(file, rank);
      switch (position.consult_legal_position(current_square).piece) {
      case Piece::Pawn:
        legal_pawn_moves(current_square);
        break;
      case Piece::Knight:
        legal_knight_moves(current_square);
        break;
      case Piece::Bishop:
        legal_bishop_moves(current_square);
        break;
      case Piece::Rook:
        legal_rook_moves(current_square);
        break;
      case Piece::Queen:
        legal_queen_moves(current_square);
        break;
      case Piece::King:
        legal_king_moves(current_square);
        break;
      default:
        break;
      }
    }
  }
}

bool MoveList::check() {
  return false; // TODO remove this, just a STUB
}

bool MoveList::double_check() {
  return false; // TODO remove this, just a STUB
}

void MoveList::legal_pawn_moves(const PiecePlacement &position) const {}

void MoveList::legal_knight_moves(const PiecePlacement &position) const {}

void MoveList::legal_bishop_moves(const PiecePlacement &position) const {}

void MoveList::legal_rook_moves(const PiecePlacement &position) const {}

void MoveList::legal_queen_moves(const PiecePlacement &position) const {}

void MoveList::legal_king_moves(const PiecePlacement &position) const {}
