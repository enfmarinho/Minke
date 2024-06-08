/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVE_GENERATION_HPP
#define MOVE_GENERATION_HPP

#include "game_elements.hpp"
#include "position.hpp"

bool under_atack(const Position &position, const PiecePlacement &sq);

class MoveList {
public:
  MoveList(const Position &position);

  Movement *begin() { return m_movement_list; };
  Movement *end() { return m_end; }

private:
  void pseudolegal_pawn_moves(const Position &position,
                              const PiecePlacement &from);
  void pseudolegal_knight_moves(const Position &position,
                                const PiecePlacement &from);
  void pseudolegal_king_moves(const Position &position,
                              const PiecePlacement &from);
  void pseudolegal_sliders_moves(const Position &position,
                                 const PiecePlacement &from);
  void sort_moves();

  Movement m_movement_list[MaxMoves], *m_end;
};

#endif // #ifndef MOVE_GENERATION_HPP
