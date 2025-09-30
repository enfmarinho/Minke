/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <cstdint>

#include "history.h"
#include "pv_list.h"
#include "time_manager.h"
#include "tt.h"
#include "types.h"

constexpr int SEE_PRUNING_MULT = -20;
constexpr int SEE_PRUNING_DEPTH = 10;
constexpr int FP_DEPTH = 8;
extern int LMR_TABLE[64][64];
constexpr int LMP_DEPTH = 11;
extern int LMP_TABLE[2][LMP_DEPTH];
constexpr int RFP_DEPTH = 10;
constexpr int RFP_MARGIN = 100;
constexpr int NMP_DEPTH = 3;
constexpr int NMP_BASE = 3;
constexpr int NMP_DIVISOR = 4;
constexpr int AW_MIN_DEPTH = 4;
constexpr int AW_DELTA = 10;

struct SearchLimits {
    int depth;
    int optimum_node;
    int maximum_node;

    inline SearchLimits();
    inline SearchLimits(int depth, int optimum_node, int maximum_node);

    inline void reset();
};

struct NodeData {
    Move curr_move;
    ScoreType static_eval;
    PvList pv_list;
    MoveList quiets_tried, tacticals_tried;

    inline void reset() {
        curr_move = MOVE_NONE;
        static_eval = SCORE_NONE;
        pv_list.clear();
        quiets_tried.clear();
        tacticals_tried.clear();
    }
};

struct ThreadData {
    TranspositionTable tt;

    Position position;
    History search_history;
    NodeData nodes[MAX_SEARCH_DEPTH];
    Move best_move;

    SearchLimits search_limits;
    TimeManager time_manager;
    int64_t nodes_searched;
    int height;
    bool stop;
    bool report;

    ThreadData();
    void reset_search_parameters();
    void set_search_limits(const SearchLimits sl);
};

ScoreType iterative_deepening(ThreadData &td);
ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td);
ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, ThreadData &td);
ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &td);
bool SEE(Position &position, const Move &move, int threshold);

#endif // #ifndef SEARCH_H
