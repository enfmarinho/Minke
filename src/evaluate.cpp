/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "evaluate.h"
#include "game_elements.h"
#include "position.h"
#include "weights.h"

WeightType eval::evaluate(const Position &position) {
  WeightType mid_game_evaluation = 0, end_game_evaluation = 0, game_state = 0;
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement pp(file, rank);
      Square square = position.consult(pp);
      if (square.piece == Piece::None)
        continue;

      WeightType piece_multiplication_factor =
          static_cast<WeightType>(square.player);
      IndexType piece_idx = piece_index(square.piece);
      if (square.player == Player::Black)
        pp = pp.mirrored();

      mid_game_evaluation +=
          (*weights::MidGamePointerTable[piece_idx])[pp.file()][pp.rank()] *
          piece_multiplication_factor;
      end_game_evaluation +=
          (*weights::EndGamePointerTable[piece_idx])[pp.file()][pp.rank()] *
          piece_multiplication_factor;
      game_state += weights::PhaseTable[piece_idx];
    }
  }
  WeightType mid_game_weight;
  if (game_state > weights::MidGamePhaseMax)
    mid_game_weight = weights::MidGamePhaseMax;
  else
    mid_game_weight = game_state;

  WeightType end_game_weight = weights::MidGamePhaseMax - mid_game_weight;
  return (mid_game_evaluation * mid_game_weight +
          end_game_evaluation * end_game_weight) /
         weights::MidGamePhaseMax; // tapered_evaluation
}
