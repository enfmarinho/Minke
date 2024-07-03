/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_state.h"
#include "evaluate.h"
#include "game_elements.h"
#include "position.h"
#include <cassert>
#include <iostream>
#include <string>

GameState::GameState() {
  // TODO check if it's worth to reserve space on m_position_stack
  // m_position_stack.reserve(200);
  reset(StartFEN);
  m_pv_index = 0;
}

bool GameState::make_move(const Move &move) {
  push();
  if (!position().move(move)) {
    pop();
    return false;
  }
  m_net.push();

  const Square &moved = position().consult(move.to);
  m_net.add_feature(moved, move.to);
  m_net.remove_feature(moved, move.from);

  if (move.move_type == MoveType::Regular) {
    // Do nothing
  } else if (move.move_type == MoveType::Capture) {
    const Square &captured =
        m_position_stack[m_position_stack.size() - 2].consult(move.to);
    m_net.remove_feature(captured, move.to);
  } else if (move.move_type == MoveType::EnPassant) {
    const Player adversary =
        (moved.player == Player::White ? Player::Black : Player::White);
    m_net.remove_feature({Piece::Pawn, adversary}, move.to);
  } else if (move.move_type == MoveType::KingSideCastling) {
    const IndexType file = (moved.player == Player::White ? 0 : 7);
    m_net.add_feature({Piece::Rook, moved.player}, PiecePlacement(file, 5));
    m_net.remove_feature({Piece::Rook, moved.player}, PiecePlacement(file, 7));
  } else if (move.move_type == MoveType::QueenSideCastling) {
    const IndexType file = (moved.player == Player::White ? 0 : 7);
    m_net.add_feature({Piece::Rook, moved.player}, PiecePlacement(file, 3));
    m_net.remove_feature({Piece::Rook, moved.player}, PiecePlacement(file, 0));
  }

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

void GameState::print_pv(int depth) const {
  for (int index = 0; index < depth; ++index)
    std::cout << m_principal_variation[index].get_algebraic_notation() << " ";
}

void GameState::set_pv(Move move) { m_principal_variation[m_pv_index] = move; }

void GameState::increase_pv_index() { ++m_pv_index; }

void GameState::decrease_pv_index() { --m_pv_index; }
