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
#include <cstdint>

bool under_attack(const Position &position, const PiecePlacement &pp,
                  const Player &player);

class MoveList {
public:
  using size_type = size_t;

  MoveList(const Position &position);
  [[nodiscard]] size_type remaining_moves() const;
  [[nodiscard]] Move next_move();

private:
  void pseudolegal_pawn_moves(const Position &position,
                              const PiecePlacement &from);
  void pseudolegal_knight_moves(const Position &position,
                                const PiecePlacement &from);
  void pseudolegal_king_moves(const Position &position,
                              const PiecePlacement &from);
  void pseudolegal_sliders_moves(const Position &position,
                                 const PiecePlacement &from);
  void pseudolegal_castling_moves(const Position &position);
  void calculate_see();

  Move m_move_list[MaxMoves], *m_end;
  WeightType m_see[MaxMoves];
  uint8_t m_start_index;
};

#endif // #ifndef MOVEGEN_H
