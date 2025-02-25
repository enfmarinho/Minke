/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>

#include "position.h"
#include "search.h"
#include "types.h"

bool under_attack(const Position &position, const Player &attacker_player, const PiecePlacement &pp_defender);
Piece cheapest_attacker(const Position &position, const Player &attacker_player, const PiecePlacement &pp_defender,
                        PiecePlacement &pp_atacker);
bool SEE(const Position &position, const Move &move);

class MoveList {
  public:
    using size_type = size_t;

    MoveList(const GameState &game_state, const Move &move);
    MoveList(const Position &position);
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_type size() const;
    [[nodiscard]] size_type n_legal_moves(Position position) const;
    [[nodiscard]] Move next_move();

  private:
    void gen_pseudolegal_moves(const Position &position);
    void pseudolegal_pawn_moves(const Position &position, const PiecePlacement &from);
    void pseudolegal_knight_moves(const Position &position, const PiecePlacement &from);
    void pseudolegal_king_moves(const Position &position, const PiecePlacement &from);
    void pseudolegal_sliders_moves(const Position &position, const PiecePlacement &from);
    void pseudolegal_castling_moves(const Position &position);
    void calculate_scores(const GameState &game_state, const Move &move);

    Move m_move_list[MaxMoves];
    WeightType m_move_scores[MaxMoves];
    uint8_t m_begin{0}, m_end{0};
};

#endif // #ifndef MOVEGEN_H
