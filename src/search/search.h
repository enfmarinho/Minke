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
#include <functional>
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

    inline void reset() {
        curr_pmove = PieceMove::none();
        excluded_move = Move::none();
        reduction = 0;
        static_eval = SCORE_NONE;
        pv_list.clear();
    }
};

struct ThreadData {
    size_t id;

    Position position;
    NNUE nnue;
    History search_history;
    CorrectionHistory correction_history;
    SearchStackEntry search_stack[MAX_SEARCH_DEPTH];
    Move best_move;

    int64_t nodes_searched;
    int64_t node_table[64 * 64];

    ThreadData();
    void reset_search_parameters();
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
    Engine() {
        // init main thread data
        m_main_thread_data = std::make_unique<ThreadData>();
        m_main_thread_data->id = 0;
        m_main_thread_data->reset_search_parameters();
    }
    ~Engine() = default;

    void prepare_search() {
        for (auto &td : m_threads_data) {
            td.reset_search_parameters();
        }
        m_main_thread_data->reset_search_parameters();
    }

    void prepare_search(const Position &pos) {
        for (auto &td : m_threads_data) {
            td.position = pos;
            td.nnue.refresh(pos);
            td.reset_search_parameters();
        }
        m_main_thread_data->position = pos;
        m_main_thread_data->nnue.refresh(pos);
        m_main_thread_data->reset_search_parameters();
    }

    std::pair<Move, ScoreType> search() {
        assert(m_threads.size() == m_threads_data.size());

        m_stop = false;
        for (size_t i = 0; i < m_threads.size(); ++i) {
            m_threads[i] = std::thread(&Engine::iterative_deepening, this, std::ref(m_threads_data[i]));
        }
        const ScoreType score = iterative_deepening(*m_main_thread_data);
        m_stop = true;

        m_tt.update_age();

        wait_until_idle(); // join helper threads

        return {m_main_thread_data->best_move, score};
    }

    void stop_search() { m_stop = true; }

    void resize_threads(size_t new_size) {
        assert(new_size >= 1);

        --new_size; // the caller thread is already a search thread, so -1
        m_threads.resize(new_size);
        m_threads_data.resize(new_size);

        // SAFETY: m_main_thread_data is guaranteed to have been allocated by the constructor, so its safe to
        // dereference it
        const ThreadData &main_td = *m_main_thread_data;
        for (size_t i = 0; i < m_threads_data.size(); ++i) {
            m_threads_data[i].id = i + 1;
            m_threads_data[i].position = main_td.position;
            m_threads_data[i].nnue = main_td.nnue;
            m_threads_data[i].reset_search_parameters();
        }
    }
    void resize_tt(size_t MB) { m_tt.resize(MB); }
    void clear_tt() { m_tt.clear(); }

    void limit_search(const SearchLimits &sl) { m_search_limiter.init(sl); }

    void new_game() {
        clear_tt();
        m_search_limiter.init();
        for (auto &m_td : m_threads_data) {
            m_td.search_history.reset();
            m_td.correction_history.reset();
            m_td.reset_search_parameters();
        }
        m_main_thread_data->search_history.reset();
        m_main_thread_data->correction_history.reset();
        m_main_thread_data->reset_search_parameters();
    }
    bool stopped() const { return m_stop; }

    void report(bool r) { m_report = r; }

    ScoreType static_eval() {
        auto &td = *m_main_thread_data;
        return td.nnue.eval(td.position);
    }

    size_t nodes_searched() const {
        size_t total_nodes = m_main_thread_data->nodes_searched;
        for (const auto &td : m_threads_data) {
            total_nodes += td.nodes_searched;
        }
        return total_nodes;
    }
    Position &position() { return m_main_thread_data->position; }
    const Position &position() const { return m_main_thread_data->position; }
    ThreadData &main_td() { return *m_main_thread_data; }
    const ThreadData &main_td() const { return *m_main_thread_data; }
    bool is_chess960() const { return m_is_chess960; }
    void set_chess960(bool c) { m_is_chess960 = c; }
    void wait_until_idle() {
        for (auto &t : m_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    static bool SEE(Position &position, const Move &move, int threshold);

  private:
    ScoreType iterative_deepening(ThreadData &td);
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
