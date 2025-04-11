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

extern int LMRTable[64][64];
constexpr int RFP_depth = 10;
constexpr int RFP_margin = 100;
constexpr int NMP_depth = 3;
constexpr int NMP_base = 3;
constexpr int NMP_divisor = 4;

struct ThreadData {
    Position position;
    TimeManager time_manager;
    History search_history;

    int64_t nodes_searched;
    int64_t node_limit;
    int searching_ply;
    int depth_limit;
    bool stop;

    void reset();
};

void iterative_deepening(ThreadData &thread_data);
ScoreType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &thread_data);
ScoreType negamax(ScoreType alpha, ScoreType beta, const CounterType &depth, PvList &pv_list, ThreadData &thread_data);
ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &thread_data);
bool SEE(Position &position, const Move &move, int threshold);

#endif // #ifndef SEARCH_H
