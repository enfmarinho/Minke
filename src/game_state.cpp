/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_state.h"
#include "game_elements.h"
#include "position.h"
#include <cassert>
#include <string>

GameState::GameState() {
  // TODO check if it's worth to reserve space on m_position_stack
  // m_position_stack.reserve(200);
  reset(StartFEN);
}

bool GameState::make_move(const Move &move) {
  push();
  if (!position().move(move)) {
    pop();
    return false;
  }
  m_net.push();
  // TODO Update nn features
  return true;
}

void GameState::undo_move() {
  assert(!m_position_stack.empty());
  pop();
  m_net.pop();
}

bool GameState::reset(const std::string &fen) {
  m_position_stack.clear();
  m_position_stack.emplace_back();

  if (!position().reset(fen))
    return false;

  m_net.reset(position());
  return true;
}

WeightType GameState::consult_history(const Move &move) const {
  return m_move_history[piece_index(position().consult(move.to).piece) +
                        (position().side_to_move() == Player::White ? 0 : 6)]
                       [move.to.index()];
}

WeightType &GameState::consult_history(const Move &move) {
  return m_move_history[piece_index(position().consult(move.from).piece) +
                        (position().side_to_move() == Player::White ? 0 : 6)]
                       [move.to.index()];
}

void GameState::increment_history(const Move &move, const CounterType &depth) {
  consult_history(move) += depth * depth;
}

WeightType GameState::eval() const {
  return m_net.eval(position().side_to_move());
}

const Position &GameState::position() const { return m_position_stack.back(); }

Position &GameState::position() { return m_position_stack.back(); }

void GameState::push() { m_position_stack.push_back(position()); }

void GameState::pop() { m_position_stack.pop_back(); }
