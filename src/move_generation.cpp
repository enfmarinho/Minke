/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move_generation.h"
#include "game_elements.h"
#include "position.h"

Movement move_generation::progressive_deepening(Position &position) {
  IndexType current = 0;
  Movement best_move = minimax(position, ++current);
  bool stop_search = false; // TODO remove this, should not be here
  while (!stop_search) {
    best_move = minimax(position, ++current);
  }
  return best_move;
}

Movement move_generation::minimax(Position &position, const IndexType &depth) {
  // TODO
  return Movement(PiecePlacement(0, 0), PiecePlacement(0, 0), Square(),
                  MoveType::Regular); // TODO remove this, just a STUB.
}
