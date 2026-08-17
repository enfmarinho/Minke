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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "core/move.h"
#include "core/position.h"
#include "core/types.h"
#include "eval/nnue.h"
#include "search/correction.h"
#include "search/history.h"
#include "search/pv_list.h"
#include "search/search_limiter.h"
#include "search/tt.h"

constexpr int LMP_DEPTH = 32;
extern int LMP_TABLE[2][LMP_DEPTH];
extern int LMR_TABLE[64][64];

struct SearchStackEntry {
    PieceMove curr_pmove;
    Move excluded_move;
    CounterType reduction;
    ScoreType static_eval;
    PvList pv_list;

    inline void init();
};

struct ThreadData {
    size_t id;

    Position position;
    NNUE nnue;
    History search_history;
    CorrectionHistory correction_history;
    SearchStackEntry search_stack[MAX_SEARCH_DEPTH];

    int64_t nodes_searched;
    int64_t node_table[64 * 64];

    void init();
    inline bool is_main() const { return id == 0; }
};

inline void make_move(ThreadData &td, const Move move) {
    DirtyPiece dp = td.position.make_move(move);
    td.nnue.push(dp, td.position.king_sq(WHITE), td.position.king_sq(BLACK));
}

inline void unmake_move(ThreadData &td, const Move move) {
    td.position.unmake_move(move);
    td.nnue.pop();
}

inline void make_null_move(ThreadData &td) { td.position.make_null_move(); }

inline void unmake_null_move(ThreadData &td) { td.position.unmake_null_move(); }

class Engine {
  public:
    Engine();
    ~Engine() = default;

    void new_game();
    void prepare_search();
    void prepare_search(const Position &pos);
    inline void limit_search(const SearchLimits &sl) { m_search_limiter.init(sl); }

    std::pair<Move, ScoreType> search();
    inline void stop_search() { m_stop = true; }
    inline bool stopped() const { return m_stop; }
    void wait_until_idle();

    void resize_threads(size_t new_size);
    void resize_tt(size_t MB) { m_tt.resize(MB); }
    void clear_tt() { m_tt.clear(); }

    void report(bool r) { m_report = r; }

    bool is_chess960() const { return m_is_chess960; }
    void set_chess960(bool c) { m_is_chess960 = c; }

    inline ScoreType static_eval() { return m_main_thread_data->nnue.eval(m_main_thread_data->position); }
    size_t nodes_searched() const;

    Position &position() { return m_main_thread_data->position; }
    const Position &position() const { return m_main_thread_data->position; }
    ThreadData &main_td() { return *m_main_thread_data; }
    const ThreadData &main_td() const { return *m_main_thread_data; }

    static bool SEE(Position &position, const Move &move, int threshold);

  private:
    std::pair<Move, ScoreType> iterative_deepening(ThreadData &td);
    ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td);
    ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, CounterType ply, const bool cutnode,
                      ThreadData &td);
    ScoreType quiescence(ScoreType alpha, ScoreType beta, CounterType ply, ThreadData &td);

    inline bool time_over(const ThreadData &td) {
        return m_stop || (td.is_main() && m_search_limiter.time_over(td.nodes_searched));
    }

    void report_search_info(const CounterType &depth, const ScoreType &eval, const PvList &pv_list,
                            const ThreadData &td);
    void report_search_result(const ThreadData &td, Move best_move);

    std::vector<std::thread> m_threads;
    std::vector<ThreadData> m_threads_data;
    std::unique_ptr<ThreadData> m_main_thread_data;
    SearchLimiter m_search_limiter;
    TranspositionTable m_tt;

    bool m_stop{true};
    bool m_report{true};
    bool m_is_chess960{false};
};
