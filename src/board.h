/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef BOARD_H
#define BOARD_H

#include "game_elements.h"
#include <stack>

class Board {
public:
  Board();
  ~Board() = default;
  void reset();
  Square consult_position(const Position &position) const;
  Square consult_position(const IndexType &file, const IndexType &rank) const;
  void move_piece(const Movement &movement);
  void undo_move();
  WeightType evaluate() const;
  MovementList get_legal_moves() const;

private:
  bool check() const;
  bool double_check() const;
  void legal_pawn_moves(const Position &position,
                        MovementList *movement_list) const;
  void legal_knight_moves(const Position &position,
                          MovementList *movement_list) const;
  void legal_bishop_moves(const Position &position,
                          MovementList *movement_list) const;
  void legal_rook_moves(const Position &position,
                        MovementList *movement_list) const;
  void legal_queen_moves(const Position &position,
                         MovementList *movement_list) const;
  void legal_king_moves(const Position &position,
                        MovementList *movement_list) const;

  Square m_board[NumberOfFiles][NumberOfRanks];
  std::stack<PastMovement> m_game_history;
  CastlingRights m_white_castling_rights;
  CastlingRights m_black_castling_rights;
  Position m_white_king_position;
  Position m_black_king_position;
  Position m_en_passant;
  CounterType m_fifty_move_counter;
  Player m_turn;
};

#endif // #ifndef BOARD_H
