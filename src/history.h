/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HISTORY_H
#define HISTORY_H

#include "move.h"
#include "position.h"
#include "types.h"

struct ThreadData;

using HistoryType = int;
constexpr HistoryType HistoryDivisor = 16384;

class History {
  public:
    History();
    ~History() = default;

    void reset();

    void update_history(const ThreadData &td, const Move &best_move, int depth, const MoveList &quiets_tried,
                        const MoveList &tacticals_tried);

    inline HistoryType get_history(const Position &position, const Move &move) const {
        return m_search_history_table[position.get_stm()][move.from_and_to()];
    }

    inline HistoryType get_capture_history(const Position &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.consult(move.from()));
        PieceType captured_pt = move.is_ep() ? PAWN : get_piece_type(position.consult(to));
        return m_capture_history[position.get_stm()][moved_pt][to][captured_pt];
    }

    inline void clear_killers(const int &depth) {
        m_killer_moves[0][depth] = MOVE_NONE;
        m_killer_moves[1][depth] = MOVE_NONE;
    }
    inline Move consult_killer1(const int &depth) const { return m_killer_moves[0][depth]; }
    inline Move consult_killer2(const int &depth) const { return m_killer_moves[1][depth]; }
    inline Move consult_counter(const Move &past_move) {
        // TODO try the usual indexing ([piece_type][to]), instead of butterfly
        if (past_move == MOVE_NONE)
            return MOVE_NONE;
        return m_counter_moves[past_move.from_and_to()];
    }
    inline bool is_killer(const Move &move, const int &depth) const {
        return move == consult_killer1(depth) || move == consult_killer2(depth);
    }

  private:
    void update_capture_history_score(const Position &position, const Move &move, int bonus);
    void update_history_heuristic_score(const Position &position, const Move &move, int bonus);

    inline void save_killer(const Move &move, const int depth) {
        m_killer_moves[1][depth] = m_killer_moves[0][depth];
        m_killer_moves[0][depth] = move;
    }

    inline void save_counter(const Move &past_move, const Move &move) {
        if (past_move != MOVE_NONE)
            m_counter_moves[past_move.from_and_to()] = move;
    }

    HistoryType m_capture_history[2][6][64][5];
    HistoryType m_search_history_table[COLOR_NB][64 * 64];
    Move m_counter_moves[64 * 64];
    Move m_killer_moves[2][MAX_SEARCH_DEPTH];
};

#endif // #ifndef HISTORY_H
