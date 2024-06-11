/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "game_elements.h"
#include "position.h"

bool under_attack(const Position &position, const PiecePlacement &pp,
                  const Player &player);

class MoveList {
public:
  MoveList(const Position &position);

  Move *begin() { return m_movement_list; };
  Move *end() { return m_end; }

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

  Move m_movement_list[MaxMoves], *m_end;
};

#endif // #ifndef MOVEGEN_H
