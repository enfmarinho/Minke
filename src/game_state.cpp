/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_state.h"
#include "game_elements.h"
#include "movegen.h"
#include "position.h"
#include <cassert>
#include <string>

GameState::GameState() {
  // TODO check if it's worth to reserve space on m_position_stack
  // m_position_stack.reserve(200);
  assert(reset(StartFEN));
}

bool GameState::repetition_draw() const {
  // Check for draw by repetition or for a repetition within the search tree
  int counter = 0;
  int distance = top().get_fifty_move_counter();
  int starting_index = m_played_positions.size();
  for (int index = 4; index <= distance; index += 2) {
    if (m_played_positions[starting_index - index] == top().get_hash()) {
      // TODO
      // if (index < depth_seaerched) {// 2-fold repetition within the search
      // tree, this avoids cycles return true;
      // }
      ++counter;
      if (counter >= 2) { // 3 fold repetition
        return true;
      }
    }
  }
  return false;
}

// Checks for a draw by the 50 move rule
bool GameState::draw_50_move() const {
  if (top().get_fifty_move_counter() >= 100) {
    Player adversary;
    PiecePlacement king_placement;
    if (top().side_to_move() == Player::White) {
      adversary = Player::Black;
      king_placement = top().white_king_position();
    } else {
      assert(top().side_to_move() == Player::Black);
      adversary = Player::White;
      king_placement = top().black_king_position();
    }

    if (!under_attack(top(), adversary, king_placement)) {
      return true; // King is safe
    }

    MoveList move_list(top());
    while (!move_list.empty()) {
      Move move = move_list.next_move();
      Position cp_pos = top();
      if (cp_pos.move(move)) {
        return true; // Not on checkmate
      }
    }

    return false; // Checkmate, since king is under attack and there is no legal
                  // move
  }

  return false;
}

bool GameState::draw() const {
  return repetition_draw() || draw_50_move() || top().insufficient_material();
}

bool GameState::make_move(const Move &move) {
  m_position_stack.push_back(top());
  if (!top().move(move)) {
    m_position_stack.pop_back();
    return false;
  }
  m_played_positions.push_back(top().get_hash());
  m_net.push();
  m_last_move = move;

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
  } else if (move.move_type == MoveType::PawnPromotionQueen ||
             move.move_type == MoveType::PawnPromotionKnight ||
             move.move_type == MoveType::PawnPromotionRook ||
             move.move_type == MoveType::PawnPromotionBishop) {
    m_net.add_feature(moved, move.from); // Add just removed feature
    m_net.remove_feature(Square(Piece::Pawn, moved.player), move.from);

    const Square &captured =
        m_position_stack[m_position_stack.size() - 2].consult(move.to);
    if (captured.piece != Piece::None) { // Capture happened
      m_net.remove_feature(captured, move.to);
    }
  }

  assert(m_net.top() == m_net.debug_func(top()));
  return true;
}

void GameState::undo_move() {
  assert(!m_position_stack.empty());
  m_position_stack.pop_back();
  m_played_positions.pop_back();
  m_net.pop();
}

bool GameState::reset(const std::string &fen) {
  Position position;
  if (!position.reset(fen))
    return false;

  std::memset(m_move_history, 0, sizeof(m_move_history));

  m_position_stack.clear();
  m_position_stack.push_back(position);

  m_played_positions.clear();
  m_played_positions.push_back(position.get_hash());

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
