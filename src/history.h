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

using HistoryType = int;
constexpr HistoryType HistoryDivisor = 16384;

class History {
  public:
    History();
    ~History() = default;

    void reset();

    void update_history(const Position &position, const MoveList &quiet_moves, const MoveList tactical_moves,
                        int depth);
    void update_capture_history(const Position &position, const MoveList tactical_moves, int depth);

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

    HistoryType m_capture_history[6][64][5];
    HistoryType m_search_history_table[COLOR_NB][64 * 64];
    Move m_killer_moves[2][MAX_SEARCH_DEPTH];
};

#endif // #ifndef HISTORY_H
