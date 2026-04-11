/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef HISTORY_H
#define HISTORY_H

#include "game_state.h"
#include "move.h"
#include "types.h"

struct ThreadData;

using HistoryType = int;
constexpr HistoryType HistoryDivisor = 16384;

class History {
  public:
    History();
    ~History() = default;

    void reset();

    void update_history(const ThreadData &td, const Move &best_move, int depth, const PieceMoveList &quiets_tried,
                        const PieceMoveList &tacticals_tried);

    HistoryType get_history(const ThreadData &td, const Move &move) const;

    inline HistoryType get_capture_history(const GameState &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.consult(move.from()));
        PieceType captured_pt = move.is_ep() ? PAWN : get_piece_type(position.consult(to));
        return m_capture_history[position.get_stm()][moved_pt][to][captured_pt];
    }

    inline void clear_killers(const int &height) {
        m_killer_moves[height][0] = MOVE_NONE;
        m_killer_moves[height][1] = MOVE_NONE;
    }
    inline Move consult_killer1(const int &height) const { return m_killer_moves[height][0]; }
    inline Move consult_killer2(const int &height) const { return m_killer_moves[height][1]; }
    inline Move consult_counter(const Move &past_move) const {
        // TODO try the usual indexing ([piece_type][to]), instead of butterfly
        if (past_move == MOVE_NONE)
            return MOVE_NONE;
        return m_counter_moves[past_move.from_and_to()];
    }
    inline bool is_killer(const Move &move, const int &height) const {
        return move == consult_killer1(height) || move == consult_killer2(height);
    }
    inline bool is_counter(const Move &move, const Move &past_move) const { return move == consult_counter(past_move); }

  private:
    void update_capture_history_score(const GameState &position, const Move &move, int bonus);
    void update_history_heuristic_score(const GameState &position, const Move &move, int bonus);
    void update_continuation_history_table(const ThreadData &td, const PieceMove &pmove, int bonus);

    void update_continuation_history_score(const ThreadData &td, const PieceMove &pmove, int bonus, int offset);
    HistoryType get_history_heuristic_score(const GameState &position, const Move &move) const;
    HistoryType get_continuation_history_score(const ThreadData &td, const PieceMove &pmove) const;
    HistoryType get_continuation_history_entry(const ThreadData &td, const PieceMove &pmove, int offset) const;

    inline void save_killer(const Move &move, const int height) {
        m_killer_moves[height][1] = m_killer_moves[height][0];
        m_killer_moves[height][0] = move;
    }

    inline void save_counter(const Move &past_move, const Move &move) {
        if (past_move != MOVE_NONE)
            m_counter_moves[past_move.from_and_to()] = move;
    }

    HistoryType m_capture_history[2][6][64][5];
    HistoryType m_search_history_table[COLOR_NB][64 * 64];
    HistoryType m_continuation_history[12 * 64][12 * 64];
    Move m_counter_moves[64 * 64];
    Move m_killer_moves[MAX_SEARCH_DEPTH][2];
};

#endif // #ifndef HISTORY_H
