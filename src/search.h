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
#include "move.h"
#include "pv_list.h"
#include "time_manager.h"
#include "tt.h"
#include "types.h"

constexpr int LMP_DEPTH = 32;
extern int LMP_TABLE[2][LMP_DEPTH];
extern int LMR_TABLE[64][64];

struct SearchLimits {
    int depth;
    int optimum_node;
    int maximum_node;

    inline SearchLimits();
    inline SearchLimits(int _depth, int _optimum_node, int _maximum_node);

    inline void reset();
};

struct NodeData {
    Move curr_move;
    Move excluded_move;
    ScoreType static_eval;
    PvList pv_list;
    MoveList quiets_tried, tacticals_tried;

    inline void reset() {
        curr_move = MOVE_NONE;
        excluded_move = MOVE_NONE;
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
    bool chess960;

    ThreadData();
    void reset_search_parameters();
    void set_search_limits(const SearchLimits sl);
};

ScoreType iterative_deepening(ThreadData &td);
ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td);
ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, const bool cutnode, ThreadData &td);
ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &td);
bool SEE(Position &position, const Move &move, int threshold);

#endif // #ifndef SEARCH_H
