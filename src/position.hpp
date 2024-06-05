/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_HPP
#define POSITION_HPP

#include "game_elements.hpp"
#include <stack>
#include <string>

class Position {
public:
  Position();
  ~Position() = default;
  bool reset(const std::string &fen);
  Square consult_position(const PiecePlacement &position) const;
  Square consult_position(const IndexType &file, const IndexType &rank) const;
  Square consult_legal_position(const PiecePlacement &position) const;
  Square consult_legal_position(const IndexType &file,
                                const IndexType &rank) const;
  Square &consult_legal_position(const PiecePlacement &position);
  Square &consult_legal_position(const IndexType &file, const IndexType &rank);
  void move(const Movement &movement);
  void undo_move();
  std::string get_algebraic_notation(const Movement &move) const;
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

private:
  Square m_board[NumberOfFiles][NumberOfRanks];
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

#endif // #ifndef POSITION_HPP
