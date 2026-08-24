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

    void update(const ThreadData &td, Move best_move, int depth, CounterType ply, const PieceMoveList &quiets_tried,
                const PieceMoveList &tacticals_tried);

    int quiet_score(const ThreadData &td, Move move, CounterType ply) const;

    inline HistoryType noisy_score(const Position &position, Move move) {
        const Square to = move.to();
        const PieceType moved_pt = get_piece_type(position.piece_at(move.from()));
        PieceType captured_pt = get_piece_type(position.piece_at(to));
        if (captured_pt == NONE)
            captured_pt = PAWN;
        return m_noisy_history[position.stm()][moved_pt][to][captured_pt][position.is_threatened(to)].value;
    }

    inline void clear_killers(int height) {
        m_killer_moves[height][0] = Move::none();
        m_killer_moves[height][1] = Move::none();
    }

    inline Move consult_killer1(int height) const { return m_killer_moves[height][0]; }

    inline Move consult_killer2(int height) const { return m_killer_moves[height][1]; }

    inline bool is_killer(Move move, int height) const {
        return move == consult_killer1(height) || move == consult_killer2(height);
    }

  private:
    struct HistoryEntry {
        HistoryType value{};

        inline void update_score(int bonus) { value += bonus - value * std::abs(bonus) / HISTORY_DIVISOR; }
        inline void update_with_base(int bonus, int base) { value += bonus - base * std::abs(bonus) / HISTORY_DIVISOR; }
    };

    void update_noisy_history_score(const Position &position, Move move, int bonus);
    void update_quiet_history_score(const Position &position, Move move, int bonus);
    void update_continuation_history_scores(const ThreadData &td, PieceMove pmove, int bonus, CounterType ply);
    void update_continuation_history_score(const ThreadData &td, PieceMove pmove, int bonus, int base, CounterType ply,
                                           int offset);

    HistoryType quiet_history_score(const Position &position, Move move) const;
    int continuation_history_score(const ThreadData &td, PieceMove pmove, CounterType ply) const;
    HistoryType continuation_history_entry(const ThreadData &td, PieceMove pmove, CounterType ply, int offset) const;

    inline void save_killer(Move move, int height) {
        m_killer_moves[height][1] = m_killer_moves[height][0];
        m_killer_moves[height][0] = move;
    }

    HistoryEntry m_noisy_history[2][6][64][5][2];
    HistoryEntry m_quiet_history[2][64 * 64][2][2];
    HistoryEntry m_continuation_history[12 * 64][12 * 64];
    Move m_killer_moves[MAX_SEARCH_DEPTH][2];
};
