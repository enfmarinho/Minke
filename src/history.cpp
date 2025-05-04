/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "history.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

inline HistoryType calculate_bonus(const int depth) {
    // Gravity formula taken from Berseck
    return std::min(1729, 4 * depth * depth + 164 * depth - 113);
}

inline void update_score(HistoryType *value, const int bonus) {
    *value += bonus - *value * std::abs(bonus) / HistoryDivisor;
}

History::History() { reset(); }

void History::reset() {
    std::memset(m_search_history_table, 0, sizeof(m_search_history_table));
    std::memset(m_killer_moves, MOVE_NONE.internal(), sizeof(m_killer_moves));
};

void History::update_history(const Position &position, const MoveList &quiet_moves, const MoveList tactical_moves,
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

void History::update_capture_history(const Position &position, const MoveList tactical_moves, int depth) {
    Move best_move = tactical_moves.moves[tactical_moves.size - 1];
    HistoryType bonus = calculate_bonus(depth);
    HistoryType *value = underlying_capture_history(position, best_move);
    update_score(value, bonus);

    for (int idx = 0; idx < tactical_moves.size - 1; ++idx) {
        value = underlying_capture_history(position, tactical_moves.moves[idx]);
        update_score(value, -bonus);
    }
}
