/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "move.h"
#include "position.h"
#include "types.h"

using HistoryType = int;
constexpr HistoryType HistoryDivisor = 16384;

class History {
  public:
    inline History() { reset(); }
    ~History() = default;

    inline void reset() {
        std::memset(m_search_history_table, 0, sizeof(m_search_history_table));
        std::memset(m_killer_moves, MOVE_NONE.internal(), sizeof(m_killer_moves));
    };

    inline void update_history(const Position &position, const MoveList &quiet_moves, const MoveList tactical_moves,
                               int depth) {
        HistoryType bonus = calculate_bonus(depth);
        Move best_move = quiet_moves.moves[quiet_moves.size - 1];

        save_killer(best_move, depth);

        // Increase the score of the move that caused the beta cutoff
        HistoryType *value = underlying_history_heuristic(position, best_move);
        update_score(value, bonus);

        // Decrease all the quiet moves scores that did not caused a beta cutoff
        for (int idx = 0; idx < quiet_moves.size - 1; ++idx) {
            value = underlying_history_heuristic(position, quiet_moves.moves[idx]);
            update_score(value, -bonus);
        }

        // Decrease all the noisy moves scores that did not caused a beta cutoff
        for (int idx = 0; idx < tactical_moves.size; ++idx) {
            value = underlying_capture_history(position, tactical_moves.moves[idx]);
            update_score(value, -bonus);
        }
    }

    inline void update_capture_history(const Position &position, const MoveList tactical_moves, int depth) {
        Move best_move = tactical_moves.moves[tactical_moves.size - 1];
        HistoryType bonus = calculate_bonus(depth);
        HistoryType *value = underlying_capture_history(position, best_move);
        update_score(value, bonus);

        for (int idx = 0; idx < tactical_moves.size - 1; ++idx) {
            value = underlying_capture_history(position, tactical_moves.moves[idx]);
            update_score(value, -bonus);
        }
    }

    inline HistoryType get_history(const Position &position, const Move &move) const {
        return m_search_history_table[position.get_stm()][move.from_and_to()];
    }

    inline HistoryType get_capture_history(const Position &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.consult(move.from()));
        PieceType captured_pt = get_piece_type(position.consult(to));
        return m_capture_history[moved_pt][to][captured_pt];
    }

    inline Move consult_killer1(const int &depth) const { return m_killer_moves[0][depth]; }
    inline Move consult_killer2(const int &depth) const { return m_killer_moves[1][depth]; }
    inline bool is_killer(const Move &move, const int &depth) const {
        return move == consult_killer1(depth) || move == consult_killer2(depth);
    }

  private:
    inline HistoryType calculate_bonus(const int depth) {
        // Gravity formula taken from Berseck
        return std::min(1729, 4 * depth * depth + 164 * depth - 113);
    }

    inline void save_killer(const Move &move, const int depth) {
        m_killer_moves[1][depth] = m_killer_moves[0][depth];
        m_killer_moves[0][depth] = move;
    }

    HistoryType *underlying_history_heuristic(const Position &position, const Move &move) {
        return &m_search_history_table[position.get_stm()][move.from_and_to()];
    }

    HistoryType *underlying_capture_history(const Position &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.consult(move.from()));
        PieceType captured_pt = get_piece_type(position.consult(to));
        return &m_capture_history[moved_pt][to][captured_pt];
    }

    inline void update_score(HistoryType *value, const int bonus) {
        *value += bonus - *value * std::abs(bonus) / HistoryDivisor;
    }

    HistoryType m_capture_history[6][64][5];
    HistoryType m_search_history_table[COLOR_NB][64 * 64];
    Move m_killer_moves[2][MAX_SEARCH_DEPTH];
};

#endif // #ifndef HISTORY_H
