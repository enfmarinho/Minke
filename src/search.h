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

struct ThreadData {
    Position position;
    TimeManager time_manager;
    History search_history;
    Move best_move;

    int64_t nodes_searched;
    int64_t node_limit;
    int height;
    int depth_limit;
    bool stop;

    void reset_search_parameters();
};

void iterative_deepening(ThreadData &td);
ScoreType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &td);
ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, PvList &pv_list, ThreadData &td);
ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &td);
bool SEE(Position &position, const Move &move, int threshold);

#endif // #ifndef SEARCH_H
