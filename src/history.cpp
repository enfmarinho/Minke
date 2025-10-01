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
#include "types.h"

inline HistoryType calculate_bonus(const int depth) {
    // Gravity formula taken from Berseck
    return std::min(1729, 4 * depth * depth + 164 * depth - 113);
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
    const PieceMoveList quiets_tried = node.quiets_tried;
    const PieceMoveList tacticals_tried = node.tacticals_tried;

    HistoryType bonus = calculate_bonus(depth);
    if (best_move.is_quiet()) {
        save_killer(best_move, depth);
        if (td.height > 0)
            save_counter(td.nodes[td.height - 1].curr_pmove.move, best_move);

        // Increase the score of the move that caused the beta cutoff
        update_history_heuristic_score(td.position, best_move, bonus);
        update_continuation_history_table(td, td.nodes[td.height].curr_pmove, bonus);

        // Decrease all the quiet moves scores that did not caused a beta cutoff
        for (int idx = 0; idx < quiets_tried.size - 1; ++idx) {
            update_history_heuristic_score(td.position, quiets_tried.list[idx].move, -bonus);
            update_continuation_history_table(td, quiets_tried.list[idx], -bonus);
        }

    } else {
        update_capture_history_score(td.position, best_move, bonus);
    }

    // Decrease all the noisy moves scores that did not caused a beta cutoff
    for (int idx = 0; idx < tacticals_tried.size; ++idx) {
        Move move = tacticals_tried.list[idx].move;
        if (move != best_move)
            update_capture_history_score(td.position, move, -bonus);
    }
}

void History::update_capture_history_score(const Position &position, const Move &move, int bonus) {
    Square to = move.to();
    PieceType moved_pt = get_piece_type(position.consult(move.from()));
    PieceType captured_pt = move.is_ep() ? PAWN : get_piece_type(position.consult(to));
    HistoryType *ptr = &m_capture_history[moved_pt][to][captured_pt];
    update_score(ptr, bonus);
}

void History::update_history_heuristic_score(const Position &position, const Move &move, int bonus) {
    HistoryType *ptr = &m_search_history_table[position.get_stm()][move.from_and_to()];
    update_score(ptr, bonus);
}

void History::update_continuation_history_table(const ThreadData &td, const PieceMove &pmove, int bonus) {
    update_continuation_history_score(td, pmove, bonus, 1);
    update_continuation_history_score(td, pmove, bonus, 2);
    update_continuation_history_score(td, pmove, bonus, 4);
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

HistoryType History::get_history(const ThreadData &td, const PieceMove &pmove) {
    return get_history_heuristic_score(td.position, pmove.move) + 2 * get_continuation_history_score(td, pmove);
}

HistoryType History::get_history_heuristic_score(const Position &position, const Move &move) const {
    return m_search_history_table[position.get_stm()][move.from_and_to()];
}

HistoryType History::get_continuation_history_score(const ThreadData &td, const PieceMove &pmove) const {
    return get_continuation_history_ply(td, pmove, 1) + get_continuation_history_ply(td, pmove, 2) +
           get_continuation_history_ply(td, pmove, 4);
}

HistoryType History::get_continuation_history_ply(const ThreadData &td, const PieceMove &pmove, int offset) const {
    int past_node_idx = td.height - offset;
    if (past_node_idx < 0)
        return 0;

    const int &past_piece_to_idx = static_cast<int>(td.nodes[past_node_idx].curr_pmove.piece) *
                                   static_cast<int>(td.nodes[past_node_idx].curr_pmove.move.to());
    const int &curr_piece_to_idx = static_cast<int>(pmove.piece) * static_cast<int>(pmove.move.to());
    return m_continuation_history[past_piece_to_idx][curr_piece_to_idx];
}
