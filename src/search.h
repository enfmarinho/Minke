/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "pv_list.h"
#include "time_manager.h"
#include "types.h"

struct SearchData {
    GameState game_state;
    TimeManager time_manager;

    int nodes_searched;
    int searching_depth;
    int node_limit;
    int depth_limit;
    bool stop;

    void reset();
};

void iterative_deepening(SearchData &search_data);
WeightType aspiration(const CounterType &depth, PvList &pv_list, SearchData &search_data);
WeightType quiescence(WeightType alpha, WeightType beta, SearchData &search_data);
WeightType alpha_beta(WeightType alpha, WeightType beta, const CounterType &depth, PvList &pv_list,
                      SearchData &search_data);

#endif // #ifndef SEARCH_H
