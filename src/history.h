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

    void update_history(const ThreadData &td, const Move &best_move, int depth, const PieceMoveList &quiets_tried,
                        const PieceMoveList &tacticals_tried);

    HistoryType get_history(const ThreadData &td, const Move &move) const;

    inline HistoryType get_capture_history(const Position &position, const Move &move) {
        Square to = move.to();
        PieceType moved_pt = get_piece_type(position.consult(move.from()));
        PieceType captured_pt = move.is_ep() ? PAWN : get_piece_type(position.consult(to));
        return m_capture_history[position.get_stm()][moved_pt][to][captured_pt];
    }

    inline void clear_killers(const int &height) {
        m_killer_moves[0][height] = MOVE_NONE;
        m_killer_moves[1][height] = MOVE_NONE;
    }
    inline Move consult_killer1(const int &height) const { return m_killer_moves[0][height]; }
    inline Move consult_killer2(const int &height) const { return m_killer_moves[1][height]; }
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
    void update_capture_history_score(const Position &position, const Move &move, int bonus);
    void update_history_heuristic_score(const Position &position, const Move &move, int bonus);
    void update_continuation_history_table(const ThreadData &td, const PieceMove &pmove, int bonus);

    void update_continuation_history_score(const ThreadData &td, const PieceMove &pmove, int bonus, int offset);
    HistoryType get_history_heuristic_score(const Position &position, const Move &move) const;
    HistoryType get_continuation_history_score(const ThreadData &td, const PieceMove &pmove) const;
    HistoryType get_continuation_history_entry(const ThreadData &td, const PieceMove &pmove, int offset) const;

    inline void save_killer(const Move &move, const int height) {
        m_killer_moves[1][height] = m_killer_moves[0][height];
        m_killer_moves[0][height] = move;
    }

    inline void save_counter(const Move &past_move, const Move &move) {
        if (past_move != MOVE_NONE)
            m_counter_moves[past_move.from_and_to()] = move;
    }

    HistoryType m_capture_history[2][6][64][5];
    HistoryType m_search_history_table[COLOR_NB][64 * 64];
    HistoryType m_continuation_history[12 * 64][12 * 64];
    Move m_counter_moves[64 * 64];
    Move m_killer_moves[2][MAX_SEARCH_DEPTH];
};

#endif // #ifndef HISTORY_H
