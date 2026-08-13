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

#include <cstdint>

#include "core/move.h"
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
        static_eval = SCORE_NONE;
        pv_list.clear();
    }
};

struct ThreadData {
    TranspositionTable tt;

    Position position;
    NNUE nnue;
    History search_history;
    CorrectionHistory correction_history;
    SearchStackEntry search_stack[MAX_SEARCH_DEPTH];
    Move best_move;

    SearchLimiter search_limiter;
    int64_t nodes_searched;
    int64_t node_table[64 * 64];
    bool stop;
    bool datagen;
    bool report;
    bool chess960;

    ThreadData();
    void reset_search_parameters();
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

ScoreType normalize_score(ScoreType score);

ScoreType iterative_deepening(ThreadData &td);
ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td);
ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, CounterType ply, const bool cutnode,
                  ThreadData &td);
ScoreType quiescence(ScoreType alpha, ScoreType beta, CounterType ply, ThreadData &td);
bool SEE(Position &position, const Move &move, int threshold);
