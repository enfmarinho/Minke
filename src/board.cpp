/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "board.h"
#include "game_elements.h"
#include "weights.h"

Board::Board()
    : m_white_castling_rights(CastlingRights::BothSides),
      m_black_castling_rights(CastlingRights::BothSides),
      m_white_king_position(3, 3), m_black_king_position(3, 3),
      m_en_passant(0, 0), m_fifty_move_counter(0), m_turn(Player::White) {
  reset(); // initializes m_board
}

void Board::reset() {
  // TODO
}

Square Board::consult_position(const Position &position) const {
  return consult_position(position.file, position.rank);
}

Square Board::consult_position(const IndexType &file,
                               const IndexType &rank) const {
  return m_board[file + FileOffset][rank + RankOffset];
}

void Board::move_piece(const Movement &movement) {
  // TODO
}


MovementList Board::get_legal_moves() const {
  MovementList movement_list;
  return movement_list; // Just a STUB.
}

bool Board::check() const {
  return false; // TODO remove this, just a STUB.
}

bool Board::double_check() const {
  return false; // TODO remove this, just a STUB.
}

void Board::legal_pawn_moves(const Position &position,
                             MovementList *movement_list) const {}

void Board::legal_bishop_moves(const Position &position,
                               MovementList *movement_list) const {}

void Board::legal_knight_moves(const Position &position,
                               MovementList *movement_list) const {}

void Board::legal_rook_moves(const Position &position,
                             MovementList *movement_list) const {}

void Board::legal_queen_moves(const Position &position,
                              MovementList *movement_list) const {}

void Board::legal_king_moves(const Position &position,
                             MovementList *movement_list) const {}
