/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVE_GENERATION_H
#define MOVE_GENERATION_H

#include "game_elements.hpp"
#include "position.hpp"

class MoveList {
public:
  MoveList(const Position &position);
  bool check();
  bool double_check();

private:
  void legal_pawn_moves(const PiecePlacement &position) const;
  void legal_knight_moves(const PiecePlacement &position) const;
  void legal_bishop_moves(const PiecePlacement &position) const;
  void legal_rook_moves(const PiecePlacement &position) const;
  void legal_queen_moves(const PiecePlacement &position) const;
  void legal_king_moves(const PiecePlacement &position) const;

  Movement m_movement_list[MaxMoves], *last;
};

#endif // #ifndef MOVE_GENERATION_H
