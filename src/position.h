/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_H
#define POSITION_H

#include "game_elements.h"
#include <string>

class Position {
public:
  Position();
  ~Position() = default;
  bool reset(const std::string &fen);
  Square consult(const PiecePlacement &placement) const;
  Square consult(const IndexType &file, const IndexType &rank) const;
  Square &consult(const PiecePlacement &placement);
  Square &consult(const IndexType &file, const IndexType &rank);
  WeightType consult_history(const PiecePlacement &to) const;
  WeightType &consult_history(const PiecePlacement &to);
  void increment_history(const PiecePlacement &to, const CounterType &depth);
  bool move(const Move &movement);
  void undo_move();
  Move get_movement(const std::string &algebraic_notation) const;
  const Player &side_to_move() const;
  const CastlingRights &white_castling_rights() const;
  const CastlingRights &black_castling_rights() const;
  const IndexType &en_passant_rank() const;
  const PastMove &last_move() const;
  const PiecePlacement &black_king_position() const;
  const PiecePlacement &white_king_position() const;
  const HashType &get_hash() const;
  const CounterType &get_half_move_counter() const;
  std::string get_fen() const;
  void print() const;

private:
  Square m_board[0x80];
  PastMove m_game_history[HalfMovesPerMatch];
  WeightType m_move_history[NumberOfPieces * 2][0x80];
  CastlingRights m_white_castling_rights;
  CastlingRights m_black_castling_rights;
  PiecePlacement m_white_king_position;
  PiecePlacement m_black_king_position;
  IndexType m_en_passant; //!< Rank of possible en passant move
  CounterType
      m_fifty_move_counter_ply; //!<  Move counter since last irreversible move
  Player m_side_to_move;
  HashType m_hash;              //!< Zobrist hash of this position.
  CounterType m_game_clock_ply; //!< Count all the half moves made in the match
};

#endif // #ifndef POSITION_H
