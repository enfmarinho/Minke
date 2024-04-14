/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef BOARD_H
#define BOARD_H

#include "game_elements.h"
#include "weights.h"
#include <cstdint>
#include <stack>
#include <vector>

class Board {
public:
  Board();
  ~Board() = default;
  /// Resets the board to the starting state.
  void reset();
  /// Consults the board square at "position".
  Square consult_position(const Position &position) const;
  /// Consults the board square at "file" and "rank".
  Square consult_position(const IndexType &file, const IndexType &rank) const;
  /// Performs a movement in the board.
  void move_piece(const Movement &movement);
  /// Undo last move done.
  void undo_move();
  /// Consults the current value of the board.
  WeightType evaluate() const;
  /// Gets a list of all the legal moves at the current state.
  MovementList get_legal_moves() const;

private:
  /// Consults if current player is in check.
  bool check() const;
  /// Consults if current player is in double check.
  bool double_check() const;
  /**
   * Append "movement_list" with the legal moves for the pawn at "position".
   * @param position position of the pawn.
   * @param movement_list movement list to be appended.
   */
  void legal_pawn_moves(const Position &position,
                        MovementList *movement_list) const;
  /**
   * Append "movement_list" with the legal moves for the knight at "position".
   * @param position position of the knight.
   * @param movement_list movement list to be appended.
   */
  void legal_knight_moves(const Position &position,
                          MovementList *movement_list) const;
  /**
   * Append "movement_list" with the legal moves for the bishop at "position".
   * @param position position of the bishop.
   * @param movement_list movement list to be appended.
   */
  void legal_bishop_moves(const Position &position,
                          MovementList *movement_list) const;
  /**
   * Append "movement_list" with the legal moves for the rook at "position".
   * @param position position of the rook.
   * @param movement_list movement list to be appended.
   */
  void legal_rook_moves(const Position &position,
                        MovementList *movement_list) const;
  /**
   * Append "movement_list" with the legal moves for the queen at "position".
   * @param position position of the queen.
   * @param movement_list movement list to be appended.
   */
  void legal_queen_moves(const Position &position,
                         MovementList *movement_list) const;
  /**
   * Append "movement_list" with the legal moves for the king at "position".
   * @param position position of the king.
   * @param movement_list movement list to be appended.
   */
  void legal_king_moves(const Position &position,
                        MovementList *movement_list) const;

  Square m_board[NumberOfFiles][NumberOfRanks];
  std::stack<PastMovement> m_move_history;
  CastlingRights m_white_castling_rights;
  CastlingRights m_black_castling_rights;
  Position m_white_king_position;
  Position m_black_king_position;
  Position m_en_passant;
  int m_fifty_move_counter;
  Player m_turn;
};

#endif // #ifndef BOARD_H
