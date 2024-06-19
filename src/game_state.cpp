/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_state.h"
#include "game_elements.h"
#include <cassert>
#include <string>

GameState::GameState() { reset(StartFEN); }

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

void GameState::reset(const std::string &fen) {
  m_position_stack.clear();
  m_position_stack.emplace_back();

  position().reset(fen);
  m_net.reset(position());
}

WeightType GameState::eval() const {
  return m_net.eval(position().side_to_move());
}

const Position &GameState::position() const { return m_position_stack.back(); }

Position &GameState::position() { return m_position_stack.back(); }

void GameState::push() { m_position_stack.push_back(position()); }

void GameState::pop() { m_position_stack.pop_back(); }
