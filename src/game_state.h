/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "game_elements.h"
#include "nnue.h"
#include "position.h"
#include <vector>

// TODO check if its better to have a stack with past positions or rely on
// undo_move and PastMove stack.
class GameState {
public:
  GameState();
  ~GameState() = default;

  bool make_move(const Move &move);
  void undo_move();

  void reset(const std::string &fen);

  WeightType eval() const;
  const Position &position() const;
  Position &position();

private:
  void push();
  void pop();

  std::vector<Position> m_position_stack;
  Network m_net;
};

#endif // #ifndef GAME_STATE_H
