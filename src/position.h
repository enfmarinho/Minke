/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_H
#define POSITION_H

#include <cstdint>
#include <string>
#include <vector>

#include "move.h"
#include "nnue.h"
#include "types.h"

class Position {
  public:
    Position() = default;
    ~Position() = default;
    bool reset(const std::string &fen);
    Square consult(const PiecePlacement &placement) const;
    Square consult(const IndexType &file, const IndexType &rank) const;
    Square &consult(const PiecePlacement &placement);
    Square &consult(const IndexType &file, const IndexType &rank);
    bool move(const Move &move);
    Move get_movement(const std::string &algebraic_notation) const;
    const Player &side_to_move() const;
    const CastlingRights &white_castling_rights() const;
    const CastlingRights &black_castling_rights() const;
    const IndexType &en_passant_rank() const;
    const PiecePlacement &black_king_position() const;
    const PiecePlacement &white_king_position() const;
    const CounterType &material_count(const Piece &piece, const Player &player) const;
    const HashType &get_hash() const;
    const CounterType &get_half_move_counter() const;
    const CounterType &get_fifty_move_counter() const;
    std::string get_fen() const;
    void print() const;
    bool insufficient_material() const;

  private:
    Square m_board[0x80];
    CastlingRights m_white_castling_rights;
    CastlingRights m_black_castling_rights;
    PiecePlacement m_white_king_position;
    PiecePlacement m_black_king_position;
    IndexType m_en_passant;               //!< Rank of possible en passant move
    CounterType m_fifty_move_counter_ply; //!< Move counter since last irreversible move
    Player m_side_to_move;
    CounterType m_material_count[2][5];
    CounterType m_total_material_count;
    HashType m_hash;              //!< Zobrist hash of this position.
    CounterType m_game_clock_ply; //!< Count all the half moves made in the match
};

#endif // #ifndef POSITION_H
