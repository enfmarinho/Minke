/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move_generation.hpp"
#include "game_elements.hpp"
#include "position.hpp"
#include "thread.hpp"

Movement move_generation::progressive_deepening(Position &position,
                                                Thread &thread) {
  IndexType current_depth = 0;
  Movement best_move = minimax(position, ++current_depth);
  while (!thread.should_stop()) {
    best_move = minimax(position, ++current_depth);
  }
  return best_move;
}

Movement move_generation::minimax(Position &position, const IndexType &depth) {
  // TODO
  return Movement(PiecePlacement(0, 0), PiecePlacement(0, 0), Square(),
                  MoveType::Regular); // TODO remove this, just a STUB.
}
