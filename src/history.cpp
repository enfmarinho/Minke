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

#include "search.h"
#include "tune.h"

inline HistoryType calculate_score(const int depth, const int bonus_mult, const int bonus_offset, const int bonus_max) {
    return std::min(depth * bonus_mult + bonus_offset, bonus_max);
}

inline void update_score(HistoryType *value, const int bonus) {
    *value += bonus - *value * std::abs(bonus) / HistoryDivisor;
}

History::History() { reset(); }

void History::reset() {
    std::memset(m_capture_history, 0, sizeof(m_capture_history));
    std::memset(m_search_history_table, 0, sizeof(m_search_history_table));
    std::memset(m_killer_moves, MOVE_NONE.internal(), sizeof(m_killer_moves));
};

void History::update_history(const ThreadData &td, const Move &best_move, int depth) {
    const NodeData &node = td.nodes[td.height];
    const MoveList quiets_tried = node.quiets_tried;
    const MoveList tacticals_tried = node.tacticals_tried;

    HistoryType quiet_bonus = calculate_score(depth, hist_bonus_mult(), hist_bonus_offset(), hist_bonus_max());
    HistoryType quiet_penalty = calculate_score(depth, hist_penalty_mult(), hist_penalty_offset(), hist_penalty_max());
    HistoryType capture_bonus =
        calculate_score(depth, capt_hist_bonus_mult(), capt_hist_bonus_offset(), capt_hist_bonus_max());
    HistoryType capture_penalty =
        calculate_score(depth, capt_hist_penalty_mult(), capt_hist_penalty_offset(), capt_hist_penalty_max());
    if (best_move.is_quiet()) {
        save_killer(best_move, depth);
        if (td.height > 0)
            save_counter(td.nodes[td.height - 1].curr_move, best_move);

        // Increase the score of the move that caused the beta cutoff
        update_history_heuristic_score(td.position, best_move, quiet_bonus);

        // Decrease all the quiet moves scores that did not caused a beta cutoff
        for (int idx = 0; idx < quiets_tried.size - 1; ++idx) {
            update_history_heuristic_score(td.position, quiets_tried.moves[idx], quiet_penalty);
        }

    } else {
        update_capture_history_score(td.position, best_move, capture_bonus);
    }

    // Decrease all the noisy moves scores that did not caused a beta cutoff
    for (int idx = 0; idx < tacticals_tried.size; ++idx) {
        Move move = tacticals_tried.moves[idx];
        if (move != best_move)
            update_capture_history_score(td.position, move, capture_penalty);
    }
}

void History::update_capture_history_score(const Position &position, const Move &move, int bonus) {
    Square to = move.to();
    PieceType moved_pt = get_piece_type(position.consult(move.from()));
    PieceType captured_pt = move.is_ep() ? PAWN : get_piece_type(position.consult(to));
    HistoryType *ptr = &m_capture_history[position.get_stm()][moved_pt][to][captured_pt];
    update_score(ptr, bonus);
}

void History::update_history_heuristic_score(const Position &position, const Move &move, int bonus) {
    HistoryType *ptr = &m_search_history_table[position.get_stm()][move.from_and_to()];
    update_score(ptr, bonus);
}
