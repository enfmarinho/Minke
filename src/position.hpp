/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_HPP
#define POSITION_HPP

#include "game_elements.hpp"
#include <stack>

class Position {
public:
  Position();
  ~Position() = default;
  void reset();
  Square consult_position(const PiecePlacement &position) const;
  Square consult_position(const IndexType &file, const IndexType &rank) const;
  Square consult_legal_position(const PiecePlacement &position) const;
  Square consult_legal_position(const IndexType &file,
                                const IndexType &rank) const;
  Square &consult_legal_position(const PiecePlacement &position);
  Square &consult_legal_position(const IndexType &file, const IndexType &rank);
  void move_piece(const Movement &movement);
  void undo_move();
  MovementList get_legal_moves() const;
  Player side_to_move() const;
  CastlingRights white_castling_rights() const;
  CastlingRights black_castling_rights() const;
  IndexType en_passant_rank() const;
  PastMovement last_move() const;

private:
  bool check() const;
  bool double_check() const;
  void legal_pawn_moves(const PiecePlacement &position,
                        MovementList *movement_list) const;
  void legal_knight_moves(const PiecePlacement &position,
                          MovementList *movement_list) const;
  void legal_bishop_moves(const PiecePlacement &position,
                          MovementList *movement_list) const;
  void legal_rook_moves(const PiecePlacement &position,
                        MovementList *movement_list) const;
  void legal_queen_moves(const PiecePlacement &position,
                         MovementList *movement_list) const;
  void legal_king_moves(const PiecePlacement &position,
                        MovementList *movement_list) const;

  Square m_board[NumberOfFiles][NumberOfRanks];
  std::stack<PastMovement> m_game_history;
  CastlingRights m_white_castling_rights;
  CastlingRights m_black_castling_rights;
  PiecePlacement m_white_king_position;
  PiecePlacement m_black_king_position;
  IndexType m_en_passant;
  CounterType m_fifty_move_counter;
  Player m_side_to_move;
};

#endif // #ifndef POSITION_HPP
