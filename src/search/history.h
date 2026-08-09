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

#pragma once

#include "core/move.h"
#include "core/position.h"
#include "core/types.h"

struct ThreadData;

constexpr HistoryType HISTORY_DIVISOR = 16384;

class History {
  public:
    History() = default;
    ~History() = default;

    void reset();

    void update_history(const ThreadData &td, const Move &best_move, int depth, const PieceMoveList &quiets_tried,
                        const PieceMoveList &tacticals_tried);

    int get_history(const ThreadData &td, const Move &move) const;

    inline HistoryType get_capture_history(const Position &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.piece_at(move.from()));
        PieceType captured_pt = get_piece_type(position.piece_at(to));
        if (captured_pt == NONE)
            captured_pt = PAWN;
        return m_capture_history[position.stm()][moved_pt][to][captured_pt][position.is_threatened(to)].value;
    }

    inline void clear_killers(const int &height) {
        m_killer_moves[height][0] = Move::none();
        m_killer_moves[height][1] = Move::none();
    }
    inline Move consult_killer1(const int &height) const { return m_killer_moves[height][0]; }
    inline Move consult_killer2(const int &height) const { return m_killer_moves[height][1]; }
    inline Move consult_counter(const Move &past_move) const {
        if (!past_move)
            return Move::none();
        return m_counter_moves[past_move.from_and_to()];
    }
    inline bool is_killer(const Move &move, const int &height) const {
        return move == consult_killer1(height) || move == consult_killer2(height);
    }
    inline bool is_counter(const Move &move, const Move &past_move) const { return move == consult_counter(past_move); }

  private:
    struct HistoryEntry {
        HistoryType value{};

        inline void update_score(int bonus) { value += bonus - value * std::abs(bonus) / HISTORY_DIVISOR; }
    };

    void update_capture_history_score(const Position &position, const Move &move, int bonus);
    void update_history_heuristic_score(const Position &position, const Move &move, int bonus);
    void update_continuation_history_table(const ThreadData &td, const PieceMove &pmove, int bonus);

    void update_continuation_history_score(const ThreadData &td, const PieceMove &pmove, int bonus, int offset);
    HistoryType get_history_heuristic_score(const Position &position, const Move &move) const;
    int get_continuation_history_score(const ThreadData &td, const PieceMove &pmove) const;
    HistoryType get_continuation_history_entry(const ThreadData &td, const PieceMove &pmove, int offset) const;

    inline void save_killer(const Move &move, const int height) {
        m_killer_moves[height][1] = m_killer_moves[height][0];
        m_killer_moves[height][0] = move;
    }

    inline void save_counter(const Move &past_move, const Move &move) {
        if (past_move)
            m_counter_moves[past_move.from_and_to()] = move;
    }

    HistoryEntry m_capture_history[2][6][64][5][2];
    HistoryEntry m_search_history_table[2][64 * 64][2][2];
    HistoryEntry m_continuation_history[12 * 64][12 * 64];
    Move m_counter_moves[64 * 64];
    Move m_killer_moves[MAX_SEARCH_DEPTH][2];
};
