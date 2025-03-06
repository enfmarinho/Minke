/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <cstdint>

#include "pv_list.h"
#include "time_manager.h"
#include "types.h"

struct ThreadData {
    Position position;
    TimeManager time_manager;

    int64_t nodes_searched;
    int64_t node_limit;
    int searching_depth;
    int depth_limit;
    bool stop;

    void reset();
};

void iterative_deepening(ThreadData &thread_data);
WeightType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &thread_data);
WeightType quiescence(WeightType alpha, WeightType beta, ThreadData &thread_data);
WeightType alpha_beta(WeightType alpha, WeightType beta, const CounterType &depth, PvList &pv_list,
                      ThreadData &thread_data);
bool SEE(Position &position, const Move &move);

#endif // #ifndef SEARCH_H
