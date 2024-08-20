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
#include <cstddef>
#include <vector>

// TODO check if its better to have a stack with past positions or rely on
// undo_move and PastMove stack.
class GameState {
public:
  GameState();
  ~GameState() = default;

  bool make_move(const Move &move);
  void undo_move();
  bool reset(const std::string &fen);
  WeightType consult_history(const Move &move) const;
  WeightType &consult_history(const Move &move);
  void increment_history(const Move &move, const CounterType &depth);
  WeightType eval() const;
  const Position &position() const;
  Position &position();

private:
  void push();
  void pop();

  std::vector<Position> m_position_stack;
  WeightType m_move_history[NumberOfPieces * 2][0x80];
  Network m_net;
};

#endif // #ifndef GAME_STATE_H
