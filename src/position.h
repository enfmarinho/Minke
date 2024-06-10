/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_H
#define POSITION_H

#include "game_elements.h"
#include <stack>
#include <string>

class Position {
public:
  Position();
  ~Position() = default;
  bool reset(const std::string &fen);
  Square consult_legal_position(const PiecePlacement &placement) const;
  Square consult_legal_position(const IndexType &file,
                                const IndexType &rank) const;
  Square &consult_legal_position(const PiecePlacement &placement);
  Square &consult_legal_position(const IndexType &file, const IndexType &rank);
  bool move(const Movement &movement);
  Movement get_movement(const std::string &algebraic_notation) const;
  const Player &side_to_move() const;
  const CastlingRights &white_castling_rights() const;
  const CastlingRights &black_castling_rights() const;
  const IndexType &en_passant_rank() const;
  const PastMovement &last_move() const;
  const PiecePlacement &black_king_position() const;
  const PiecePlacement &white_king_position() const;
  const HashType &get_hash() const;
  const CounterType &get_half_move_counter() const;
  std::string get_fen() const;
  bool in_bounds(const PiecePlacement &placement);
  void print_board() const;

private:
  Square m_board[0x80];
  std::stack<PastMovement> m_game_history;
  CastlingRights m_white_castling_rights;
  CastlingRights m_black_castling_rights;
  PiecePlacement m_white_king_position;
  PiecePlacement m_black_king_position;
  IndexType m_en_passant;
  CounterType m_fifty_move_counter_ply;
  Player m_side_to_move;
  HashType m_hash;
  CounterType m_game_clock_ply; //!< Count all the half moves made in the match
};

#endif // #ifndef POSITION_H
