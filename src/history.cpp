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

#include "move.h"
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
    std::memset(m_continuation_history, 0, sizeof(m_continuation_history));
    std::memset(m_killer_moves, MOVE_NONE.internal(), sizeof(m_killer_moves));
    for (Move &move : m_counter_moves)
        move = MOVE_NONE;
};

HistoryType History::get_history(const ThreadData &td, const Move &move) const {
    PieceMove pmove = {move, td.position.consult(move.from())};
    return get_history_heuristic_score(td.position, move) + get_continuation_history_score(td, pmove);
}

void History::update_history(const ThreadData &td, const Move &best_move, int depth, const PieceMoveList &quiets_tried,
                             const PieceMoveList &tacticals_tried) {
    HistoryType quiet_bonus = calculate_score(depth, hist_bonus_mult(), hist_bonus_offset(), hist_bonus_max());
    HistoryType quiet_penalty = calculate_score(depth, hist_penalty_mult(), hist_penalty_offset(), hist_penalty_max());
    HistoryType cont_bonus = calculate_score(depth, cont_bonus_mult(), cont_bonus_offset(), cont_bonus_max());
    HistoryType cont_penalty = calculate_score(depth, cont_penalty_mult(), cont_penalty_offset(), cont_penalty_max());
    HistoryType capture_bonus =
        calculate_score(depth, capt_hist_bonus_mult(), capt_hist_bonus_offset(), capt_hist_bonus_max());
    HistoryType capture_penalty =
        calculate_score(depth, capt_hist_penalty_mult(), capt_hist_penalty_offset(), capt_hist_penalty_max());
    if (best_move.is_quiet()) {
        save_killer(best_move, td.height);
        if (td.height > 0)
            save_counter(td.nodes[td.height - 1].curr_pmove.move, best_move);

        // Increase the score of the move that caused the beta cutoff
        update_history_heuristic_score(td.position, best_move, quiet_bonus);
        update_continuation_history_table(td, quiets_tried.list[quiets_tried.size - 1], cont_bonus);

        // Decrease all the quiet moves scores that did not caused a beta cutoff
        for (int idx = 0; idx < quiets_tried.size - 1; ++idx) {
            update_history_heuristic_score(td.position, quiets_tried.list[idx].move, quiet_penalty);
            update_continuation_history_table(td, quiets_tried.list[idx], cont_penalty);
        }

    } else {
        update_capture_history_score(td.position, best_move, capture_bonus);
    }

    // Decrease all the noisy moves scores that did not caused a beta cutoff
    for (int idx = 0; idx < tacticals_tried.size; ++idx) {
        Move move = tacticals_tried.list[idx].move;
        if (move != best_move)
            update_capture_history_score(td.position, move, capture_penalty);
    }
}

void History::update_capture_history_score(const Position &position, const Move &move, int bonus) {
    Square to = move.to();
    PieceType moved_pt = get_piece_type(position.consult(move.from()));
    PieceType captured_pt = get_piece_type(position.consult(to));
    if (move.is_ep() || move.is_promotion())
        captured_pt = PAWN;
    HistoryType *ptr = &m_capture_history[position.get_stm()][moved_pt][to][captured_pt];
    update_score(ptr, bonus);
}

void History::update_history_heuristic_score(const Position &position, const Move &move, int bonus) {
    HistoryType *ptr = &m_search_history_table[position.get_stm()][move.from_and_to()]
                                              [position.is_threaded(move.from())][position.is_threaded(move.to())];
    update_score(ptr, bonus);
}

void History::update_continuation_history_table(const ThreadData &td, const PieceMove &pmove, int bonus) {
    update_continuation_history_score(td, pmove, bonus, 1); // Counter Moves History
    update_continuation_history_score(td, pmove, bonus, 2); // Follow Up History
    // update_continuation_history_score(td, pmove, bonus, 4);
}

void History::update_continuation_history_score(const ThreadData &td, const PieceMove &pmove, int bonus, int offset) {
    int past_node_idx = td.height - offset;
    if (past_node_idx >= 0 && td.nodes[past_node_idx].curr_pmove != PIECE_MOVE_NONE) {
        // those index are equivalent to moved_piece * to_sq
        const int &past_moved_piece_to_idx = static_cast<int>(td.nodes[past_node_idx].curr_pmove.piece) *
                                             static_cast<int>(td.nodes[past_node_idx].curr_pmove.move.to());
        const int &curr_moved_piece_to_idx = static_cast<int>(pmove.piece) * static_cast<int>(pmove.move.to());
        HistoryType *ptr = &m_continuation_history[past_moved_piece_to_idx][curr_moved_piece_to_idx];
        update_score(ptr, bonus);
    }
}

HistoryType History::get_history_heuristic_score(const Position &position, const Move &move) const {
    return m_search_history_table[position.get_stm()][move.from_and_to()][position.is_threaded(move.from())]
                                 [position.is_threaded(move.to())];
}

HistoryType History::get_continuation_history_score(const ThreadData &td, const PieceMove &pmove) const {
    return get_continuation_history_entry(td, pmove, 1) + get_continuation_history_entry(td, pmove, 2);
    // + get_continuation_history_ply(td, pmove, 4);
}

HistoryType History::get_continuation_history_entry(const ThreadData &td, const PieceMove &pmove, int offset) const {
    int past_node_idx = td.height - offset;
    if (past_node_idx < 0)
        return 0;

    const int past_piece_to_idx = static_cast<int>(td.nodes[past_node_idx].curr_pmove.piece) *
                                  static_cast<int>(td.nodes[past_node_idx].curr_pmove.move.to());
    const int curr_piece_to_idx = static_cast<int>(pmove.piece) * static_cast<int>(pmove.move.to());
    return m_continuation_history[past_piece_to_idx][curr_piece_to_idx];
}
