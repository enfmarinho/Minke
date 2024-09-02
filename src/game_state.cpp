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
  assert(reset(StartFEN));
}

bool GameState::make_move(const Move &move) {
  m_position_stack.push_back(top());
  if (!top().move(move)) {
    m_position_stack.pop_back();
    return false;
  }
  m_net.push();
  m_last_move = move;

  Piece pawn_promoted_to = Piece::None;
  const Square &moved = top().consult(move.to);
  m_net.add_feature(moved, move.to);
  m_net.remove_feature(moved, move.from);

  if (move.move_type == MoveType::Regular) {
    // Do nothing
  } else if (move.move_type == MoveType::Capture) {
    const Square &captured =
        m_position_stack[m_position_stack.size() - 2].consult(move.to);
    m_net.remove_feature(captured, move.to);
  } else if (move.move_type == MoveType::EnPassant) {
    const IndexType pawn_offset =
        moved.player == Player::White ? offsets::North : offsets::South;
    const Player adversary =
        moved.player == Player::White ? Player::Black : Player::White;
    m_net.remove_feature(
        {Piece::Pawn, adversary},
        PiecePlacement(move.to.file() - pawn_offset, move.to.rank()));
  } else if (move.move_type == MoveType::KingSideCastling) {
    const IndexType file = (moved.player == Player::White ? 0 : 7);
    m_net.add_feature({Piece::Rook, moved.player}, PiecePlacement(file, 5));
    m_net.remove_feature({Piece::Rook, moved.player}, PiecePlacement(file, 7));
  } else if (move.move_type == MoveType::QueenSideCastling) {
    const IndexType file = (moved.player == Player::White ? 0 : 7);
    m_net.add_feature({Piece::Rook, moved.player}, PiecePlacement(file, 3));
    m_net.remove_feature({Piece::Rook, moved.player}, PiecePlacement(file, 0));
  } else if (move.move_type == MoveType::PawnPromotionQueen) {
    pawn_promoted_to = Piece::Queen;
  } else if (move.move_type == MoveType::PawnPromotionKnight) {
    pawn_promoted_to = Piece::Knight;
  } else if (move.move_type == MoveType::PawnPromotionRook) {
    pawn_promoted_to = Piece::Rook;
  } else if (move.move_type == MoveType::PawnPromotionBishop) {
    pawn_promoted_to = Piece::Bishop;
  }

  if (pawn_promoted_to != Piece::None) {
    m_net.remove_feature(moved, move.to); // Remove just added feature
    m_net.add_feature({pawn_promoted_to, moved.player}, move.to);
    const Square &to_before_move =
        m_position_stack[m_position_stack.size() - 2].consult(move.to);
    if (to_before_move.piece != Piece::None) { // Capture happened
      m_net.remove_feature(to_before_move, move.to);
    }
  }

  return true;
}

void GameState::undo_move() {
  assert(!m_position_stack.empty());
  m_position_stack.pop_back();
  m_net.pop();
}

bool GameState::reset(const std::string &fen) {
  Position position;
  if (!position.reset(fen))
    return false;

  m_position_stack.clear();
  m_position_stack.push_back(position);
  m_net.reset(position);
  m_last_move = MoveNone;

  return true;
}

WeightType GameState::consult_history(const Move &move) const {
  return m_move_history[piece_index(top().consult(move.to).piece) +
                        (top().side_to_move() == Player::White ? 0 : 6)]
                       [move.to.index()];
}

WeightType &GameState::consult_history(const Move &move) {
  return m_move_history[piece_index(top().consult(move.from).piece) +
                        (top().side_to_move() == Player::White ? 0 : 6)]
                       [move.to.index()];
}

void GameState::increment_history(const Move &move, const CounterType &depth) {
  consult_history(move) += depth * depth;
}

WeightType GameState::eval() const { return m_net.eval(top().side_to_move()); }

const Position &GameState::top() const { return m_position_stack.back(); }

Position &GameState::top() { return m_position_stack.back(); }

const Move &GameState::last_move() const { return m_last_move; }
